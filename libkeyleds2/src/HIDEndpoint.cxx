/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017-2019 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "libkeyleds/HIDEndpoint.h"

#include "config.h"
#include "libkeyleds/RingBuffer.h"
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <unistd.h>
#include "uv.h"
#include <vector>

#include <iostream>

using std::literals::chrono_literals::operator""ms;

namespace libkeyleds::hid {

/****************************************************************************/

class Endpoint::implementation
{
    struct State;

    struct FilterEntry {
        void *          ref;        ///< unique handle (used to unregister)
        frame_filter    callback;   ///< function to call
    };

    struct Command {
        uv_buf_t                frame;
        frame_filter  receiveCb;
        event_handler errorCb;

        Command(Frame frame, frame_filter && receiveCb, event_handler && errorCb)
          : frame(uv_buf_init(reinterpret_cast<char *>(frame.data), frame.size)),
            receiveCb(std::move(receiveCb)),
            errorCb(std::move(errorCb))
        {}
    };

public:
            implementation(uv_loop_t &, FileDescriptor, std::size_t maxReportSize);
            implementation(const implementation &) = delete;
    implementation & operator=(const implementation &) = delete;
            ~implementation();
    void    scheduleDeletion();

    void    setTimeout(milliseconds val) noexcept { m_timeoutDelay = val; }

    void    registerFrameFilter(void * ref, frame_filter);
    void    unregisterFrameFilter(void * ref);
    bool    post(Frame, frame_filter && receiveCb, event_handler && errorCb) noexcept;

private:
    void    onFrameReceived();

private:
    // Configuration and lifecycle
    uv_loop_t &             m_loop;
    FileDescriptor          m_fd;
    milliseconds            m_timeoutDelay = 100ms;
    std::vector<FilterEntry> m_filters;

    // Reading
    uv_poll_t               m_inWatch;
    std::vector<uint8_t>    m_readBuffer;

    // Writing
    RingBuffer<Command, 4>  m_queue;
    uv_fs_t                 m_outRequest;
    uv_timer_t              m_commandTimeout;

    const State *           m_state = nullptr;
    unsigned                m_deleteRef = 0;
};

/****************************************************************************/
// Exported symbols for Endpoint wrapper

KEYLEDS_EXPORT Endpoint::Endpoint(uv_loop_t & loop, FileDescriptor fd, std::size_t maxReportSize)
 : m_impl(new implementation(loop, std::move(fd), maxReportSize))
{}

KEYLEDS_EXPORT Endpoint::~Endpoint()
{
    m_impl.release()->scheduleDeletion();
}

KEYLEDS_EXPORT void Endpoint::setTimeout(milliseconds val)
    { m_impl->setTimeout(val); }
KEYLEDS_EXPORT void Endpoint::registerFrameFilter(void * ref, frame_filter filter)
    { m_impl->registerFrameFilter(ref, std::move(filter)); }
KEYLEDS_EXPORT void Endpoint::unregisterFrameFilter(void * ref)
    { m_impl->unregisterFrameFilter(ref); }
KEYLEDS_EXPORT bool Endpoint::post(Frame frame, frame_filter receiveCb, event_handler errorCb) noexcept
    { return m_impl->post(frame, std::move(receiveCb), std::move(errorCb)); }
KEYLEDS_EXPORT void Endpoint::flush()
    { /* TODO */ }

/****************************************************************************/
// Local helpers

namespace {

template <typename T> static T * uv_get_data(const uv_fs_t * handle) { return static_cast<T*>(handle->data); }
template <typename T> static T * uv_get_data(const uv_handle_t * handle) { return static_cast<T*>(handle->data); }
template <typename T> static T * uv_get_data(const uv_poll_t * handle) { return static_cast<T*>(handle->data); }
template <typename T> static T * uv_get_data(const uv_timer_t * handle) { return static_cast<T*>(handle->data); }

}

/****************************************************************************/

struct Endpoint::implementation::State
{
    virtual void enter(implementation &) const {}
    virtual void sendNextCommand(implementation &) const { assert(false); }
    virtual void onFrameReceived(implementation &, Frame) const { }
    virtual void onCommandSent(implementation &, uv_fs_t &) const { assert(false); }
    virtual void onTimeout(implementation &) const { assert(false); }
    virtual void scheduleDeletion(implementation &) const {}

    template <typename To>
    static void transition(implementation & impl)
    {
        static constexpr To instance{};
        impl.m_state = &instance;
        instance.enter(impl);
    }

    struct Inactive;
    struct Sending;
    struct SendingPendingDelete;
    struct SendingReplyReceived;
    struct AwaitingReply;
};

struct Endpoint::implementation::State::Inactive final : public State
{
    void enter(implementation & impl) const override
    {
        std::cerr <<"entering Inactive" <<std::endl;
        if (!impl.m_queue.empty()) { sendNextCommand(impl); }
    }

    void sendNextCommand(implementation & impl) const override
    {
        uv_fs_write(&impl.m_loop, &impl.m_outRequest, impl.m_fd.get(),
                    &impl.m_queue.front().frame, 1, -1, [](uv_fs_t * req){
            auto * self = uv_get_data<implementation>(req);
            uv_fs_req_cleanup(req);
            self->m_state->onCommandSent(*self, *req);
        });
        uv_timer_start(&impl.m_commandTimeout, [](uv_timer_t * handle){
            auto * self = uv_get_data<implementation>(handle);
            self->m_state->onTimeout(*self);
        }, impl.m_timeoutDelay.count(), 0u);

        transition<Sending>(impl);
    }
};

struct Endpoint::implementation::State::Sending final : public State
{
    void enter(implementation &) const override { std::cerr <<"entering Sending" <<std::endl; }

    void onFrameReceived(implementation & impl, Frame frame) const override
    {
        std::cerr <<"Sending::onFrameReceived" <<std::endl;
        if (impl.m_queue.front().receiveCb(frame)) {
            uv_cancel(reinterpret_cast<uv_req_t *>(&impl.m_outRequest));
            transition<SendingReplyReceived>(impl);
        }
    }

    void onCommandSent(implementation & impl, uv_fs_t & req) const override
    {
        std::cerr <<"Sending::onCommandSent" <<std::endl;
        if (!uv_is_active(reinterpret_cast<uv_handle_t*>(&impl.m_commandTimeout))) {
            req.result = UV_ETIMEDOUT;
        }
        if (req.result < 0) {
            impl.m_queue.front().errorCb(static_cast<int>(req.result));
            uv_timer_stop(&impl.m_commandTimeout);
            impl.m_queue.pop();
            transition<Inactive>(impl);
        } else {
            transition<AwaitingReply>(impl);
        }
    }

    void onTimeout(implementation & impl) const override
    {
        std::cerr <<"Sending::onTimeout" <<std::endl;
        uv_cancel(reinterpret_cast<uv_req_t *>(&impl.m_outRequest));
    }

    void scheduleDeletion(implementation & impl) const override
    {
        std::cerr <<"Sending::scheduleDeletion" <<std::endl;
        impl.m_deleteRef += 1;
        uv_cancel(reinterpret_cast<uv_req_t *>(&impl.m_outRequest));
        impl.m_queue.front().errorCb(UV_ECANCELED);
        transition<SendingPendingDelete>(impl);
    }
};

struct Endpoint::implementation::State::SendingReplyReceived final : public State
{
    void enter(implementation &) const override { std::cerr <<"entering SendingReplyReceived" <<std::endl; }

    void onCommandSent(implementation & impl, uv_fs_t &) const override
    {
        std::cerr <<"SendingReplyReceived::onCommandSent" <<std::endl;
        uv_timer_stop(&impl.m_commandTimeout);
        impl.m_queue.pop();
        transition<Inactive>(impl);
    }

    void onTimeout(implementation &) const override
    {
        std::cerr <<"SendingReplyReceived::onTimeout" <<std::endl;
    }

    void scheduleDeletion(implementation & impl) const override
    {
        std::cerr <<"SendingReplyReceived::scheduleDeletion" <<std::endl;
        impl.m_deleteRef += 1;
        transition<SendingPendingDelete>(impl);
    }
};

struct Endpoint::implementation::State::AwaitingReply final : public State
{
    void enter(implementation &) const override { std::cerr <<"entering AwaitingReply" <<std::endl; }

    void onFrameReceived(implementation & impl, Frame frame) const override
    {
        std::cerr <<"AwaitingReply::onFrameReceived" <<std::endl;
        if (impl.m_queue.front().receiveCb(frame)) {
            uv_timer_stop(&impl.m_commandTimeout);
            impl.m_queue.pop();
            transition<Inactive>(impl);
        }
    }

    void onTimeout(implementation & impl) const override
    {
        std::cerr <<"AwaitingReplyState::onTimeout" <<std::endl;
        impl.m_queue.front().errorCb(UV_ETIMEDOUT);
        impl.m_queue.pop();
        transition<Inactive>(impl);
    }
};

struct Endpoint::implementation::State::SendingPendingDelete final : public State {
    void enter(implementation &) const override { std::cerr <<"entering SendingPendingDelete" <<std::endl; }
    void onCommandSent(implementation & impl, uv_fs_t &) const override
    {
        std::cerr <<"SendingPendingDelete::onCommandSent" <<std::endl;
        impl.m_queue.pop();
        if (--impl.m_deleteRef == 0) { delete &impl; }
    }
};


/****************************************************************************/

Endpoint::implementation::implementation(uv_loop_t & loop, FileDescriptor fd, std::size_t maxReportSize)
 : m_loop(loop), m_fd(std::move(fd)), m_readBuffer(maxReportSize)
{
    uv_poll_init(&m_loop, &m_inWatch, m_fd.get());
    m_inWatch.data = this;
    uv_poll_start(&m_inWatch, UV_READABLE, [](uv_poll_t * handle, int, int) {
        uv_get_data<implementation>(handle)->onFrameReceived();
    });

    m_outRequest.type = UV_UNKNOWN_REQ;
    m_outRequest.data = this;

    uv_timer_init(&m_loop, &m_commandTimeout);
    m_commandTimeout.data = this;

    State::transition<State::Inactive>(*this);
}

Endpoint::implementation::~implementation()
{
    while (!m_queue.empty()) {
        m_queue.front().errorCb(UV_ECANCELED);
        m_queue.pop();
    }
}

void Endpoint::implementation::scheduleDeletion()
{
    assert(m_deleteRef == 0);

    m_deleteRef += 1;
    uv_close(reinterpret_cast<uv_handle_t *>(&m_inWatch), [](uv_handle_t * handle) {
        auto * self = uv_get_data<implementation>(handle);
        if (--self->m_deleteRef == 0) { delete self; }
    });

    m_deleteRef += 1;
    uv_close(reinterpret_cast<uv_handle_t *>(&m_commandTimeout), [](uv_handle_t * handle) {
        auto * self = uv_get_data<implementation>(handle);
        if (--self->m_deleteRef == 0) { delete self; }
    });
    m_state->scheduleDeletion(*this);
}

void Endpoint::implementation::registerFrameFilter(void * ref, frame_filter filter)
{
    auto it = std::find_if(m_filters.begin(), m_filters.end(),
                           [ref](const auto & filter) { return filter.ref == ref; });
    if (it == m_filters.end()) {
        m_filters.push_back({ ref, filter });
    } else {
        it->callback = filter;
    }
}

void Endpoint::implementation::unregisterFrameFilter(void * ref)
{
    auto it = std::find_if(m_filters.begin(), m_filters.end(),
                           [ref](const auto & filter) { return filter.ref == ref; });
    if (it != m_filters.end()) {
        m_filters.erase(it);
    }
}

bool Endpoint::implementation::post(Frame frame, frame_filter && receiveCb, event_handler && errorCb) noexcept
{
    if (m_queue.size() == m_queue.capacity()) { return false; }
    m_queue.emplace(frame, std::move(receiveCb), std::move(errorCb));
    if (m_queue.size() == 1) { m_state->sendNextCommand(*this); }
    return true;
}

void Endpoint::implementation::onFrameReceived()
{
    ssize_t nread;
    nread = read(m_fd.get(), m_readBuffer.data(), m_readBuffer.size());
    if (nread < 0) { return; }

    auto data = Frame{m_readBuffer.data(), static_cast<unsigned>(nread)};
    for (const auto & filter : m_filters) { if (filter.callback(data)) { return; } }

    m_state->onFrameReceived(*this, data);
}

/****************************************************************************/

} // namespace libkeyleds::hid
