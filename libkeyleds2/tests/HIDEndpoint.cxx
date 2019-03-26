/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
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

#include "libkeyleds/utils.h"

#include "AsyncTest.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <uv.h>


namespace hid = libkeyleds::hid;
using FileDescriptor = libkeyleds::FileDescriptor;

/****************************************************************************/

class HIDEndpoint : public ::testing::AsyncTest
{
protected:
    static constexpr std::size_t maxReportSize = 64;
protected:
    void SetUp() override
    {
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets) < 0) {
            FAIL() <<"Could not create socketpair for testing";
        }
        fcntl(sockets[0], F_SETFL, O_NONBLOCK);
        endpointFd = FileDescriptor(sockets[0]);
        deviceFd = FileDescriptor(sockets[1]);
    }

    template <std::size_t N>
    void send(const uint8_t (&data)[N])
    {
        write(deviceFd.get(), data, N);
    }

    std::vector<uint8_t> receive()
    {
        std::vector<uint8_t> result(maxReportSize);
        auto nread = read(deviceFd.get(), result.data(), result.size());
        result.resize(std::size_t(std::max(0l, nread)));
        return result;
    }

protected:
    FileDescriptor  endpointFd;
    FileDescriptor  deviceFd;
};

/****************************************************************************/

TEST_F(HIDEndpoint, basicLifecycle) {
    auto endpoint = hid::Endpoint(loop, std::move(endpointFd), maxReportSize);
}


TEST_F(HIDEndpoint, frameFilter) {
    static constexpr uint8_t message[] = {0x01, 0x02, 0x03, 0x04};

    auto endpoint = hid::Endpoint(loop, std::move(endpointFd), maxReportSize);

    // Register a frame filter that counts invocations
    auto counterA = 0u;
    endpoint.registerFrameFilter(&counterA, [&counterA](hid::Endpoint::Frame frame) {
        EXPECT_TRUE(std::equal(std::begin(message), std::end(message),
                               frame.data, frame.data + frame.size));
        counterA += 1;
        return true;
    });

    send(message);
    run_until([&]{ return counterA > 0; });
    EXPECT_EQ(counterA, 1);

    // Register a second frame filter
    auto counterB = 0u;
    endpoint.registerFrameFilter(&counterB, [&counterB](hid::Endpoint::Frame frame) {
        EXPECT_TRUE(std::equal(std::begin(message), std::end(message),
                               frame.data, frame.data + frame.size));
        counterB += 1;
        return true;
    });

    // Second filter does not get called because first returns true
    send(message);
    run_until([&]{ return counterA > 1; });
    EXPECT_EQ(counterA, 2);
    EXPECT_EQ(counterB, 0);

    // Filter no longer gets called after being unregistered, but second now does
    endpoint.unregisterFrameFilter(&counterA);
    send(message);
    run_until([&]{ return counterB > 0; });
    EXPECT_EQ(counterA, 2);
    EXPECT_EQ(counterB, 1);
}

TEST_F(HIDEndpoint, basicCommand) {
    static uint8_t query[] = {0x01, 0x02, 0x03, 0x04};
    static uint8_t answer[] = {0x11, 0x12, 0x13, 0x14};

    auto endpoint = hid::Endpoint(loop, std::move(endpointFd), maxReportSize);

    auto counterA = 0u;
    endpoint.post(hid::Endpoint::Frame{query, sizeof(query)},
        [&counterA](auto frame) {
            EXPECT_TRUE(std::equal(std::begin(answer), std::end(answer),
                                   frame.data, frame.data + frame.size));
            counterA += 1;
            return true;
        },
        [](auto error) { ADD_FAILURE() <<uv_strerror(error); }
    );

    auto result = receive();
    EXPECT_TRUE(std::equal(std::begin(query), std::end(query),
                           std::begin(result), std::end(result)));

    send(answer);
    run_until([&]{ return counterA > 0; });
    EXPECT_EQ(counterA, 1);
}

TEST_F(HIDEndpoint, commandSendTimeout) {
    static uint8_t query[] = {0x01, 0x02, 0x03, 0x04};
    auto endpoint = hid::Endpoint(loop, std::move(endpointFd), maxReportSize);

    endpoint.setTimeout(hid::Endpoint::milliseconds(0));

    bool resolved = false;
    auto counter = 0u;
    endpoint.post(hid::Endpoint::Frame{query, sizeof(query)},
        [&resolved](auto) {
            ADD_FAILURE() <<"success callback must not be called";
            resolved = true; return true;
        },
        [&counter, &resolved](auto code) {
            EXPECT_EQ(code, UV_ETIMEDOUT) <<uv_strerror(code);
            resolved = true; counter += 1;
        }
    );
    run_until([&]{ return resolved; });
    EXPECT_EQ(counter, 1);
}

TEST_F(HIDEndpoint, commandDeviceTimeout) {
    static uint8_t query[] = {0x01, 0x02, 0x03, 0x04};
    auto endpoint = hid::Endpoint(loop, std::move(endpointFd), maxReportSize);

    endpoint.setTimeout(hid::Endpoint::milliseconds(1));

    bool resolved = false;
    auto counter = 0u;
    endpoint.post(hid::Endpoint::Frame{query, sizeof(query)},
        [&resolved](auto) {
            ADD_FAILURE() <<"success callback must not be called";
            resolved = true; return true;
        },
        [&counter, &resolved](auto code) {
            EXPECT_EQ(code, UV_ETIMEDOUT) <<uv_strerror(code);
            resolved = true; counter += 1;
        }
    );
    run_until([&]{ return resolved; });
    EXPECT_TRUE(resolved);
    EXPECT_EQ(counter, 1);
}
