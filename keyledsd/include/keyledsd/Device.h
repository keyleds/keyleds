#ifndef KEYLEDSD_KEYBOARD_H
#define KEYLEDSD_KEYBOARD_H

#include <exception>
#include <memory>
#include <vector>
#include "keyleds.h"
#include "keyledsd/common.h"
#include "tools/DeviceWatcher.h"

namespace keyleds {

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
                        error(std::string what, keyleds_error_t code)
                         : std::runtime_error(what), m_code(code) {}
        keyleds_error_t code() const { return m_code; }
    private:
        keyleds_error_t m_code;
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
    void                setTimeout(unsigned us);
    bool                resync();
    void                fillColor(const KeyBlock & block, RGBColor);
    void                setColors(const KeyBlock & block, const color_directive_list &);
    void                setColors(const KeyBlock & block, const ColorDirective[], size_t size);
    color_directive_list getColors(const KeyBlock & block);
    void                commitColors();

private:
    static dev_ptr      openDevice(const std::string &);
    static Type         getType(struct keyleds_device *);
    static std::string  getName(struct keyleds_device *);
    static block_list   getBlocks(struct keyleds_device *);
    void                cacheVersion();

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
    typedef std::vector<size_t> index_list;
public:
                        KeyBlock(keyleds_block_id_t id, key_list && keys, RGBColor maxValues);

    keyleds_block_id_t  id() const { return m_id; }
    const std::string & name() const { return m_name; }
    const key_list &    keys() const { return m_keys; }
    const RGBColor &    maxValues() const { return m_maxValues; }

    index_list::value_type find(uint8_t id) const { return m_keysInverse[id]; }

private:
    keyleds_block_id_t  m_id;
    std::string         m_name;
    key_list            m_keys;
    index_list          m_keysInverse;
    RGBColor            m_maxValues;
};

/****************************************************************************/

class DeviceWatcher : public device::FilteredDeviceWatcher
{
    Q_OBJECT
public:
            DeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
    bool    isVisible(const device::Description & dev) const override;
};

/****************************************************************************/

};

#endif
