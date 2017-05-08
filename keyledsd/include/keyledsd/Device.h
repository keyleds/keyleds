#ifndef KEYLEDSD_KEYBOARD_H
#define KEYLEDSD_KEYBOARD_H

#include <stdint.h>
#include <memory>
#include "keyleds.h"
#include "tools/DeviceWatcher.h"

namespace keyleds {

struct Color {
    uint8_t red, green, blue;
};

/****************************************************************************/

class Device
{
    typedef std::unique_ptr<struct keyleds_device,
                            void(*)(struct keyleds_device *)> dev_ptr;
public:
    // Transient types
    enum Type { Keyboard, Remote, NumPad, Mouse, TouchPad, TrackBall, Presenter, Receiver };
    typedef struct keyleds_key_color ColorDirective;
    typedef std::vector<ColorDirective> color_directive_list;

    // Data
    class KeyBlock;
    typedef std::vector<KeyBlock> block_list;

    // Exceptions
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

    // Query
    int                 socket();
    Type                type() const { return m_type; }
    const std::string & name() const { return m_name; }
    const std::string & model() const { return m_model; }
    const std::string & serial() const { return m_serial; }
    const std::string & firmware() const { return m_firmware; }
          bool          hasLayout() const { return m_layout != KEYLEDS_KEYBOARD_LAYOUT_INVALID; }
          int           layout() const { return m_layout; }
    const block_list &  blocks() const { return m_blocks; }

    // Manipulate
    void                fillColor(const KeyBlock & block, Color);
    void                setColors(const KeyBlock & block, const color_directive_list &);
    color_directive_list getColors(const KeyBlock & block);
    void                commitColors();

private:
    void                cacheType();
    void                cacheName();
    void                cacheVersion();
    void                cacheLayout();
    void                loadBlocks();

private:
    dev_ptr             m_device;
    Type                m_type;
    std::string         m_name;
    std::string         m_model;
    std::string         m_serial;
    std::string         m_firmware;
    int                 m_layout;
    block_list          m_blocks;
};


class Device::KeyBlock
{
public:
    typedef std::vector<uint8_t> key_list;
public:
                        KeyBlock(keyleds_block_id_t id, key_list && keys, Color maxValues)
                            : m_id(id), m_keys(keys), m_maxValues(maxValues) {}

    keyleds_block_id_t  id() const { return m_id; }
    const key_list &    keys() const { return m_keys; }
    const Color &       maxValues() const { return m_maxValues; }

private:
    keyleds_block_id_t  m_id;
    key_list            m_keys;
    Color               m_maxValues;
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
