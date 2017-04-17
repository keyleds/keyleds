#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stddef.h>
#include <cstring>
#include <istream>
#include <sstream>
#include <memory>
#include <keyleds.h>
#include "keyledsd/Layout.h"

using keyleds::Layout;

/****************************************************************************/

typedef std::unique_ptr<xmlParserCtxt, void(*)(xmlParserCtxtPtr)> xmlParserCtxt_ptr;
typedef std::unique_ptr<xmlDoc, void(*)(xmlDocPtr)> xmlDoc_ptr;
typedef std::unique_ptr<xmlChar, void(*)(void *)> xmlString;

static const xmlChar KEYBOARD_TAG[] = "keyboard";
static const xmlChar ROW_TAG[] = "row";
static const xmlChar KEY_TAG[] = "key";

static const xmlChar ROOT_ATTR_NAME[] = "layout";
static const xmlChar KEYBOARD_ATTR_X[] = "x";
static const xmlChar KEYBOARD_ATTR_Y[] = "y";
static const xmlChar KEYBOARD_ATTR_WIDTH[] = "width";
static const xmlChar KEYBOARD_ATTR_HEIGHT[] = "height";
static const xmlChar KEYBOARD_ATTR_ZONE[] = "zone";
static const xmlChar KEY_ATTR_CODE[] = "code";
static const xmlChar KEY_ATTR_GLYPH[] = "glyph";
static const xmlChar KEY_ATTR_STYLE[] = "style";
static const xmlChar KEY_ATTR_WIDTH[] = "width";

/****************************************************************************/

static void hideErrorFunc(void *, xmlErrorPtr) {}

static unsigned parseUInt(const xmlNode * node, const xmlChar * name, int base)
{
    xmlString valueStr(xmlGetProp(node, name), xmlFree);
    if (valueStr == NULL) {
        std::ostringstream errMsg;
        errMsg <<"Element '" <<node->name <<"' misses a '" <<name <<"' attribute";
        throw Layout::ParseError(errMsg.str(), xmlGetLineNo(node));
    }
    char * strEnd;
    unsigned valueUInt = std::strtoul((char*)valueStr.get(), &strEnd, base);
    if (*strEnd != '\0') {
        std::ostringstream errMsg;
        errMsg <<"Value '" <<valueStr.get() <<"' in attribute '" <<name <<"' of '"
               <<node->name <<"' element cannot be parsed as an integer";
        throw Layout::ParseError(errMsg.str(), xmlGetLineNo(node));
    }
    return valueUInt;
}

static const char * resolveCode(unsigned code)
{
    unsigned keyCode = keyleds_translate_scancode(code);
    const char * name = keyleds_lookup_string(keyleds_keycode_names, keyCode);
    return name;
}

/****************************************************************************/

static void parseKeyboard(const xmlNode * keyboard, Layout::key_list & keys)
{
    char * strEnd;
    unsigned kbX = parseUInt(keyboard, KEYBOARD_ATTR_X, 10);
    unsigned kbY = parseUInt(keyboard, KEYBOARD_ATTR_Y, 10);
    unsigned kbWidth = parseUInt(keyboard, KEYBOARD_ATTR_WIDTH, 10);
    unsigned kbHeight = parseUInt(keyboard, KEYBOARD_ATTR_HEIGHT, 10);
    unsigned kbZone = parseUInt(keyboard, KEYBOARD_ATTR_ZONE, 0);

    unsigned nbRows = 0;
    for (const xmlNode * row = keyboard->children; row != NULL; row = row->next) {
        if (row->type == XML_ELEMENT_NODE || xmlStrcmp(row->name, ROW_TAG) == 0) { nbRows += 1; }
    }

    unsigned rowIdx = 0;
    for (const xmlNode * row = keyboard->children; row != NULL; row = row->next) {
        if (row->type != XML_ELEMENT_NODE || xmlStrcmp(row->name, ROW_TAG) != 0) { continue; }

        unsigned totalWidth = 0;
        for (const xmlNode * key = row->children; key != NULL; key = key->next) {
            if (key->type != XML_ELEMENT_NODE || xmlStrcmp(key->name, KEY_TAG) != 0) { continue; }
            xmlString keyWidthStr(xmlGetProp(key, KEY_ATTR_WIDTH), xmlFree);
            if (keyWidthStr != NULL) {
                float keyWidthFloat = ::strtof((char*)keyWidthStr.get(), &strEnd);
                if (*strEnd != '\0') {
                    std::ostringstream errMsg;
                    errMsg <<"Value '" <<keyWidthStr.get() <<"' in attribute '"
                           <<KEY_ATTR_WIDTH <<"' of '" <<KEY_TAG
                           <<"' element cannot be parsed as a float";
                    throw Layout::ParseError(errMsg.str(), xmlGetLineNo(key));
                }
                totalWidth += (unsigned int)(keyWidthFloat * 1000);
            } else {
                totalWidth += 1000;
            }
        }

        unsigned xOffset = 0;
        for (const xmlNode * key = row->children; key != NULL; key = key->next) {
            if (key->type != XML_ELEMENT_NODE || xmlStrcmp(key->name, KEY_TAG) != 0) { continue; }
            xmlString code(xmlGetProp(key, KEY_ATTR_CODE), xmlFree);
            xmlString glyph(xmlGetProp(key, KEY_ATTR_GLYPH), xmlFree);
            xmlString keyWidthStr(xmlGetProp(key, KEY_ATTR_WIDTH), xmlFree);

            unsigned keyWidth;
            if (keyWidthStr != NULL) {
                keyWidth = kbWidth
                         * (unsigned int)(::strtof((char*)keyWidthStr.get(), &strEnd) * 1000)
                         / totalWidth;
            } else {
                keyWidth = kbWidth * 1000 / totalWidth;
            }

            if (code != nullptr) {
                unsigned codeVal = parseUInt(key, KEY_ATTR_CODE, 0);
                const char * const codeName = glyph != nullptr
                                            ? (const char*)glyph.get()
                                            : resolveCode(codeVal);
                keys.emplace_back(
                    kbZone,
                    codeVal,
                    Layout::Rect {
                        kbX + xOffset,
                        kbY + rowIdx * (kbHeight / nbRows),
                        kbX + xOffset + keyWidth - 1,
                        kbY + (rowIdx + 1) * (kbHeight / nbRows) - 1
                    },
                    codeName != NULL ? std::string(codeName) : std::string()
                );
            }
            xOffset += keyWidth;
        }
        rowIdx += 1;
    }
}

/****************************************************************************/

void Layout::parse(std::istream & stream)
{
    // Parser context
    xmlParserCtxt_ptr context(xmlNewParserCtxt(), xmlFreeParserCtxt);
    if (context == NULL) {
        throw std::runtime_error("Failed to initialize libxml");
    }
    xmlSetStructuredErrorFunc(context.get(), hideErrorFunc);

    // Document
    std::ostringstream bufferStream;
    bufferStream << stream.rdbuf();
    std::string buffer = bufferStream.str();
    xmlDoc_ptr document(
        xmlCtxtReadMemory(context.get(), buffer.data(), buffer.size(),
                          NULL, NULL, XML_PARSE_NOWARNING | XML_PARSE_NONET),
        xmlFreeDoc
    );
    if (document == NULL) {
        auto error = xmlCtxtGetLastError(context.get());
        if (error == NULL) { throw Layout::ParseError("empty file", 1); }
        std::string errMsg(error->message);
        errMsg.erase(errMsg.find_last_not_of(" \r\n") + 1);
        throw Layout::ParseError(errMsg, error->line);
    }

    // Search for keyboard nodes
    const xmlNode * const root = xmlDocGetRootElement(document.get());
    xmlString name(xmlGetProp(root, ROOT_ATTR_NAME), xmlFree);
    m_name = std::string(reinterpret_cast<std::string::const_pointer>(name.get()));

    for (const xmlNode * node = root->children; node != NULL; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, KEYBOARD_TAG) == 0) {
            parseKeyboard(node, m_keys);
        }
    }

    // Finalize
    computeBounds();
}

void Layout::computeBounds()
{
    m_bounds = m_keys.front().position;

    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it) {
        const auto & pos = it->position;
        if (pos.x0 < m_bounds.x0) { m_bounds.x0 = pos.x0; }
        if (pos.y0 < m_bounds.y0) { m_bounds.y0 = pos.y0; }
        if (pos.x1 > m_bounds.x1) { m_bounds.x1 = pos.x1; }
        if (pos.y1 > m_bounds.y1) { m_bounds.y1 = pos.y1; }
    }
}
