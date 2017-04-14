#ifndef KEYLEDSD_PLUGIN_H
#define KEYLEDSD_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/* Common definitions */

#define KEYLEDSD_PLUGIN_MAGIC0      (0xb9092354ul)
#define KEYLEDSD_PLUGIN_MAGIC1      (0x693e5f89ul)
#define KEYLEDSD_PLUGIN_API_VERSON  (1)

typedef struct {
    unsigned        items;
    struct {
        const char *key;
        const char *value;
    }               entries[];
} KeyledsConfig;

typedef struct {
    uint8_t         red;
    uint8_t         green;
    uint8_t         blue;
    uint8_t         alpha;
} KeyledsColor;

typedef struct {
    unsigned        block_id;       /**< key block the key belongs to */
    unsigned        scancode;       /**< keyboard code for key */
    unsigned        evcode;         /**< event code for key (event device) */
    unsigned        keycode;        /**< X11 keycode */
    const char *    keyname;        /**< human-readable name for key */
    unsigned        x, y;           /**< Center of key */
    unsigned        x0, y0, x1, y1; /**< Bounding rect of key */
} KeyledsdKey;

typedef struct {
    unsigned        block_id;
    unsigned        nb_keys;
    KeyledsdKey *   keys;
} KeyledsdBlock;

typedef struct {
    char *          name;           /**< Descriptive name in English */
    char *          serial;         /**< Uniquely identifies the device */
    unsigned        type;           /**< Device type */
    unsigned        color_type;     /**< Supported colors */

    unsigned        nb_keys;
    KeyledsdKey *   keys;

    unsigned        nb_blocks;
    KeyledsdBlock * blocks;
} KeyledsdKeyboard;


typedef struct {
    const struct keyledsd_keyboard * keyboard;
    unsigned            delta;          /**< nanoseconds */
    KeyledsdColors *    colors;         /**< all keys as one stream */
    KeyledsdColors *    block_keys[];   /**< pointers into colors for each block */
} KeyledsdTarget;

/****************************************************************************/
/* API */

extern void keyledsd_start_animation(const KeyledsdKeyboard *);
extern void keyledsd_stop_animation(const KeyledsdKeyboard *);

/****************************************************************************/
/* Exports */

typedef enum {
    KEYLEDSD_PLUGIN_NORMAL = 0,
    KEYLEDSD_PLUGIN_FILTER = (1<<1)     /**< if set, render will receive previous plugin's output */
    KEYLEDSD_PLUGIN_ONCE = (1<<2)       /**< if set, keyledsd will prevent more than one use per device */
} keyledsd_plugin_type_t;

/** Exported plugin structure
 *
 * This structure is looked up in the symbol table as `modele_def` after
 * the plugin is loaded into memory.
 */
struct _keyledsd_plugin {
    uint32_t        magic[2];           /**< Used to check this is a keyledsd plugin */
    uint32_t        api_version;        /**< Must be set to KEYLEDSD_PLUGIN_API_VERSON */
    const char *    name;               /**< Plugin name in UTF-8 encoding. */
    keyledsd_plugin_type_t type;        /**< Specifies plugin options */

    /** Initialize the plugin
     *
     * Called once when the plugin is first loaded.
     * @return A pointer that will be passed unmodified as `ptr_g` to all other
     *         functions. Can be used for plugin state.
     */
    void *          (*init)();

    /** Shutdown the plugin
     *
     * Called once when the plugin is no longer used. After this function
     * returns, the service will attempt to unload the plugin from memory.
     * @param ptr_g The pointer returned by init().
     */
    void            (*shutdown)(void * ptr_g);

    /** Instanciate the plugin for a given configuration
     *
     * Called when a device is added and the plugin is part of the plugin stack
     * for the device. If specified multiple times, each specification will
     * cause one invocation of this function with corresponding configuration,
     * unless KEYLEDSD_PLUGIN_ONCE was set for the plugin.
     * @param ptr_g The pointer returned by init().
     * @param[in] conf Configuration dictionary for instanciation.
     * @param[in] device Device description of the keyboard being setup.
     * @return A pointer that will be passed unmodified as `ptr_k` to other
     *         per-configuration/per-device functions. Can be used for plugin state.
     */
    void *          (*instanciate)(void * ptr_g, const KeyledsdConfig * conf,
                                                 const KeyledsdKeyboard * device);

    /** Destroy an instance of the plugin
     *
     * Called when a device is removed. It is called exactly once for each
     * invocation of instanciate(), in reverse order.
     * @param ptr_g The pointer returned by init().
     * @param ptr_k The pointer returned by instanciate().
     * @param[in] device Device description of the removed keyboard.
     * @note By the time this function is called, the device is usually no
     *       longer accessible. This is because keyledsd reacts to the user
     *       disconnecting the device from the host.
     */
    void *          (*destroy)(void * ptr_g, void * ptr_k, const KeyledsdKeyboard * device);

    /** Render leds
     *
     * Called every few milliseconds as long as animation is enabled for the
     * device. It must render its output into target.
     * @warning This function must not block, and should avoid any expensive
     *          computation, as it will be invoked very often.
     * @param ptr_g The pointer returned by init().
     * @param ptr_k The pointer returned by instanciate().
     * @param[in,out] target The buffer to render into. If the plugin has
     *                KEYLEDSD_PLUGIN_FILTER bit set, the buffer will contain
     *                current rendering state from plugins below this one in
     *                the render state, and render() should update it in place.
     *                If KEYLEDSD_PLUGIN_FILTER is not set, the buffer is private
     *                to the plugin, will contain whatever the last invocation
     *                of render() had written in it, and will be alpha-blended
     *                over the render state once this function returns.
     */
    void            (*render)(void * ptr_g, void * ptr_k, KeyledsTarget * target);
};

#define KEYLEDSD_DEF_MODULE(name, fn_instantiate, fn_destroy, fn_) \
    extern struct _keyledsd_plugin module_def = { \
        {KEYLEDSD_PLUGIN_MAGIC0, KEYLEDSD_PLUGIN_MAGIC1}, \
        KEYLEDSD_PLUGIN_API_VERSION, #name, \
        fn_instantiate, fn_destroy \
    }

#ifdef __cplusplus
}
#endif

#endif
