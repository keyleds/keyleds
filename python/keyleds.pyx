
cimport keyleds as pykeyleds
from libc.stdio cimport sprintf
from libc.stdlib cimport free, malloc
from cpython cimport Py_INCREF, PyTuple_New, PyTuple_SET_ITEM


cdef class DeviceVersion:
    cdef readonly bytes serial
    cdef readonly unsigned transport
    cdef readonly bytes model
    cdef readonly tuple protocols

    def __repr__(self):
        return 'DeviceVersion(model=%s, serial=%s, transport=%d, protocols=%r)' %(
            self.model.hex(), self.serial.hex(), self.transport, self.protocols
        )

cdef class DeviceProtocol:
    cdef readonly object type
    cdef readonly str prefix
    cdef readonly object major
    cdef readonly object minor
    cdef readonly object build
    cdef readonly object active
    cdef readonly object product_id

    def __repr__(self):
        return 'DeviceProtocol(%d, product=0x%04x, version=\'%sv%d.%d.%x\', active=%r)' % (
            self.type, self.product_id, self.prefix, self.major, self.minor, self.build, self.active
        )

cdef class Color:
    cdef readonly int red
    cdef readonly int green
    cdef readonly int blue

    def __cinit__(self, int r, int g, int b):
        self.red = r
        self.green = g
        self.blue = b

    def __richcmp__(Color self, Color other, int op):
        if op == 2:
            return (self.red == other.red and
                    self.green == other.green and
                    self.blue == other.blue)
        elif op == 3:
            return (self.red != other.red or
                    self.green != other.green or
                    self.blue != other.blue)
        else:
            return False

    def __hash__(self):
        return (self.red, self.green, self.blue)

    def __str__(self):
        cdef char buffer[8]
        sprintf(buffer, '#%02x%02x%02x', self.red, self.green, self.blue)
        return buffer.decode('ascii')

    def __repr__(self):
        return 'Color(%s, %s, %s)' % (self.red, self.green, self.blue)

cdef class KeyColor:
    cdef readonly unsigned   keycode
    cdef readonly unsigned   id
    cdef readonly Color      color

    def __init__(self, keycode=0, key_id=0, color=Color(0, 0, 0)):
        self.keycode = keycode
        self.id = key_id
        self.color = color

    def __repr__(self):
        cdef const char * name
        name = pykeyleds.keyleds_lookup_string(pykeyleds.keyleds_keycode_names, self.keycode)
        if name is NULL:
            return 'KeyColor(%s, id=%s, %r)' % (self.keycode, self.id, self.color)
        return 'KeyColor(KEY_%s, id=%s, %r' % (name.decode('UTF-8'), self.id, self.color)


cdef class Device:
    cdef pykeyleds.Keyleds * _device
    cdef uint8_t _target_id
    cdef unsigned _gamemode_max

    cdef str path
    cdef object _protocol_version
    cdef object _handler
    cdef object _features
    cdef object _name
    cdef object _type
    cdef object _version
    cdef object _layout
    cdef object _supported_rates
    cdef object _leds

    def __cinit__(self, str path, uint8_t app_id, uint8_t target_id=0xff):
        self.path = path
        b_path = path.encode('UTF-8')
        self._device = pykeyleds.keyleds_open(b_path, app_id)
        self._target_id = target_id
        if self._device is NULL:
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

    def __dealloc__(self):
        if self._device is not NULL:
            pykeyleds.keyleds_close(self._device)

    cdef _query_protocol(self):
        cdef unsigned version
        cdef pykeyleds.keyleds_device_handler_t handler
        if not pykeyleds.keyleds_get_protocol(self._device, self._target_id,
                                              &version, &handler):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
        self._protocol_version = version
        self._handler = handler

    def close(self):
        if self._device is not NULL:
            pykeyleds.keyleds_close(self._device)
            self._device = NULL

    @property
    def fd(self):
        if self._device is NULL:
            return -1
        return pykeyleds.keyleds_device_fd(self._device)

    @property
    def protocol(self):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._protocol_version is None:
            self._query_protocol()
        return self._protocol_version

    @property
    def handler(self):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._handler is None:
            self._query_protocol()
        return self._handler

    def ping(self):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if not pykeyleds.keyleds_ping(self._device, self._target_id):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

    @property
    def features(self):
        cdef unsigned count
        cdef tuple feature_list
        cdef object f_id
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._features is None:
            count = pykeyleds.keyleds_get_feature_count(self._device, self._target_id)
            if count == 0:
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

            feature_list = PyTuple_New(count)
            for idx in range(count):
                f_id = pykeyleds.keyleds_get_feature_id(self._device, self._target_id, idx + 1)
                if f_id == 0:
                    raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
                Py_INCREF(f_id) # SET_ITEM steals the ref
                PyTuple_SET_ITEM(feature_list, idx, f_id)
            self._features = feature_list
        return self._features

    @property
    def name(self):
        cdef char * name
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._name is None:
            if not pykeyleds.keyleds_get_device_name(self._device, self._target_id, &name):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
            self._name = name.decode('UTF-8')
            free(name)
        return self._name

    @property
    def type(self):
        cdef pykeyleds.keyleds_device_type_t dev_type
        cdef const char * type_name
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._type is None:
            if not pykeyleds.keyleds_get_device_type(self._device, self._target_id, &dev_type):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

            type_name = pykeyleds.keyleds_lookup_string(pykeyleds.keyleds_device_types,
                                                        dev_type)
            self._type = type_name.decode('UTF-8')
        return self._type

    @property
    def version(self):
        cdef pykeyleds.keyleds_device_version * version
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._version is None:
            if not pykeyleds.keyleds_get_device_version(self._device, self._target_id, &version):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

            try:
                version_obj = DeviceVersion()
                version_obj.serial = (<char*>(version[0].serial))[:4]
                version_obj.transport = version[0].transport
                version_obj.model = (<char*>(version[0].model))[:6]

                protocols = PyTuple_New(version[0].length)
                for idx in range(version[0].length):
                    proto_obj = DeviceProtocol()
                    proto_obj.type = version[0].protocols[idx].type
                    proto_obj.prefix = version[0].protocols[idx].prefix.decode('ascii')
                    proto_obj.major = version[0].protocols[idx].version_major
                    proto_obj.minor = version[0].protocols[idx].version_minor
                    proto_obj.build = version[0].protocols[idx].build
                    proto_obj.active = version[0].protocols[idx].is_active
                    proto_obj.product_id = version[0].protocols[idx].product_id
                    Py_INCREF(proto_obj) # SET_ITEM steals the ref
                    PyTuple_SET_ITEM(protocols, idx, proto_obj)
                version_obj.protocols = protocols
            finally:
                free(version)
            self._version = version_obj
        return self._version

    @property
    def layout(self):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._layout is None:
            self._layout = pykeyleds.keyleds_keyboard_layout(self._device, self._target_id)
        if self._layout == pykeyleds.KEYLEDS_KEYBOARD_LAYOUT_INVALID:
            raise AttributeError('Device does not define a layout')
        return self._layout

    @property
    def report_rates(self):
        cdef unsigned * rates
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._supported_rates is None:
            if not pykeyleds.keyleds_get_reportrates(self._device, self._target_id, &rates):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
            try:
                idx = 0
                result = []
                while (rates[idx] > 0):
                    result.append(rates[idx])
                    idx += 1
                self._supported_rates = tuple(result)
            finally:
                free(rates)
        return self._supported_rates

    def get_report_rate(self):
        cdef unsigned rate
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if not pykeyleds.keyleds_get_reportrate(self._device, self._target_id, &rate):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
        return rate

    def set_report_rate(self, unsigned rate):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if not pykeyleds.keyleds_set_reportrate(self._device, self._target_id, rate):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

    @property
    def leds(self):
        cdef pykeyleds.keyleds_keyblocks_info * info
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._leds is None:
            if not pykeyleds.keyleds_get_block_info(self._device, self._target_id, &info):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
            try:
                blocks = {}
                for idx in range(info[0].length):
                    block = KeyBlock(
                        self,
                        info[0].blocks[idx].block_id,
                        info[0].blocks[idx].nb_keys,
                        Color(info[0].blocks[idx].red, info[0].blocks[idx].green, info[0].blocks[idx].blue)
                    )
                    blocks[block.name] = block
                self._leds = blocks
            finally:
                pykeyleds.keyleds_free_block_info(info)
        return self._leds

    def commit_leds(self):
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if not pykeyleds.keyleds_commit_leds(self._device, self._target_id):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

    def set_gamemode_keys(self, keys):
        cdef uint8_t * ids
        cdef size_t count = len(keys)
        if self._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if self._gamemode_max == 0:
            if not pykeyleds.keyleds_gamemode_max(self._device, self._target_id, &self._gamemode_max):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
        if count > self._gamemode_max:
            raise ValueError('Too many keys for gamemode, maximum is %s' % self._gamemode_max)

        ids = <uint8_t*>malloc(sizeof(uint8_t) * count)
        try:
            for idx, key_code in enumerate(keys):
                ids[idx] = key_code

            if (not pykeyleds.keyleds_gamemode_reset(self._device, self._target_id) or
                not pykeyleds.keyleds_gamemode_set(self._device, self._target_id, ids, count)):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
        finally:
            free(ids)

    def __repr__(self):
        state = ' <closed>' if self._device is NULL else ''
        return '%s(%r%s)' % (self.__class__.__name__, self.path, state)


cdef class KeyBlock:
    cdef Device _device
    cdef readonly str name
    cdef readonly pykeyleds.keyleds_block_id_t block_id
    cdef readonly unsigned nb_keys
    cdef readonly Color color

    def __cinit__(self, Device device, pykeyleds.keyleds_block_id_t block_id, unsigned nb_keys, Color color):
        cdef const char * name
        self._device = device
        name = pykeyleds.keyleds_lookup_string(pykeyleds.keyleds_block_id_names, block_id)
        self.name = None if name is NULL else name.decode('UTF-8')
        self.block_id = block_id
        self.nb_keys = nb_keys
        self.color = color

    def __repr__(self):
        return 'KeyBlock(%r, 0x%02x, nb_keys=%s, color=%r)' % (
            self.name, self.block_id, self.nb_keys, self.color
        )

    def get_all(self):
        cdef keyleds_key_color * keys
        cdef int start, stop, step, count

        if self._device._device is NULL:
            raise ValueError('I/O operation on closed device.')

        keys = <keyleds_key_color*>malloc(sizeof(keyleds_key_color) * self.nb_keys)
        try:
            if not pykeyleds.keyleds_get_leds(self._device._device, self._device._target_id,
                                              self.block_id, keys, 0, self.nb_keys):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))

            result = PyTuple_New(self.nb_keys)
            for idx in range(self.nb_keys):
                keycolor = KeyColor()
                keycolor.keycode = keys[idx].keycode
                keycolor.id = keys[idx].id
                keycolor.color = Color(keys[idx].red, keys[idx].green, keys[idx].blue)
                Py_INCREF(keycolor) # SET_ITEM steals the ref
                PyTuple_SET_ITEM(result, idx, keycolor)
        finally:
            free(keys)
        return result

    def set_keys(self, colors):
        cdef keyleds_key_color * keys
        cdef int start, stop, step, count

        if self._device._device is NULL:
            raise ValueError('I/O operation on closed device.')

        count = len(colors)
        keys = <keyleds_key_color*>malloc(sizeof(keyleds_key_color) * count)
        try:
            for idx in range(count):
                keys[idx].keycode = colors[idx].keycode
                keys[idx].id = colors[idx].id
                keys[idx].red = colors[idx].color.red
                keys[idx].green = colors[idx].color.green
                keys[idx].blue = colors[idx].color.blue
            if not pykeyleds.keyleds_set_leds(self._device._device, self._device._target_id,
                                              self.block_id, keys, count):
                raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
        finally:
            free(keys)

    def set_all_keys(self, Color color):
        if self._device._device is NULL:
            raise ValueError('I/O operation on closed device.')

        if not pykeyleds.keyleds_set_led_block(self._device._device, self._device._target_id,
                                               self.block_id,                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        color.red, color.green, color.blue):
            raise IOError(pykeyleds.keyleds_get_error_str().decode('UTF-8'))
