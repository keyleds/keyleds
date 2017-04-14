#ifndef KEYLEDSD_KEYBOARD_H
#define KEYLEDSD_KEYBOARD_H

#include <memory>
#include "tools/DeviceWatcher.h"
struct keyleds_device;
struct keyleds_keyblocks_info;

namespace keyleds {

/****************************************************************************/

class Device
{
    typedef std::unique_ptr<struct keyleds_device,
                            void(*)(struct keyleds_device *)> dev_ptr;
    typedef std::unique_ptr<struct keyleds_keyblocks_info,
                            void (*)(struct keyleds_keyblocks_info *)> blockinfo_ptr;
public:
    enum Type {
        Keyboard, Remote, NumPad, Mouse, TouchPad, TrackBall, Presenter, Receiver
    };

    class Key;
    class KeyBlock;

    class error : public std::runtime_error
    {
    public:
                    error(std::string what) : std::runtime_error(what) {}
    };

public:
                        Device(const std::string & path);
                        Device(const Device &) = delete;
                        Device(Device &&) = default;
    Device &            operator=(const Device &) = delete;
    Device &            operator=(Device &&) = default;

    int                 socket();
    const std::string & name() const;
    Type                type() const;
    const std::string & serial() const;

private:
    dev_ptr             m_device;
    std::string         m_name;
    Type                m_type;
    std::string         m_serial;
    blockinfo_ptr       m_blocks;
};

class Device::Key
{
public:
};

class Device::KeyBlock
{
public:
    typedef std::vector<Key> key_list;
public:
                        KeyBlock(unsigned id, key_list &&);

    unsigned            id() const { return m_id; }
    const key_list &    keys() const { return m_keys; }
private:
    unsigned            m_id;
    key_list            m_keys;
};

/****************************************************************************/

class DeviceWatcher : public device::FilteredDeviceWatcher
{
    Q_OBJECT
public:
            DeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
    bool    isVisible(const device::DeviceDescription & dev) const override;
};

/****************************************************************************/

};

#endif
