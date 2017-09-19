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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <istream>
#include <sstream>
#include <memory>
#include "keyledsd/LayoutDescription.h"

using keyleds::LayoutDescription;

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
static const xmlChar KEY_ATTR_WIDTH[] = "width";

/****************************************************************************/

static void hideErrorFunc(void *, xmlErrorPtr) {}

static unsigned parseUInt(const xmlNode * node, const xmlChar * name, int base)
{
    xmlString valueStr(xmlGetProp(const_cast<xmlNode *>(node), name), xmlFree);
    if (valueStr == nullptr) {
        std::ostringstream errMsg;
        errMsg <<"Element '" <<node->name <<"' misses a '" <<name <<"' attribute";
        throw LayoutDescription::ParseError(errMsg.str(), xmlGetLineNo(const_cast<xmlNode *>(node)));
    }
    char * strEnd;
    unsigned valueUInt = std::strtoul((char*)valueStr.get(), &strEnd, base);
    if (*strEnd != '\0') {
        std::ostringstream errMsg;
        errMsg <<"Value '" <<valueStr.get() <<"' in attribute '" <<name <<"' of '"
               <<node->name <<"' element cannot be parsed as an integer";
        throw LayoutDescription::ParseError(errMsg.str(), xmlGetLineNo(const_cast<xmlNode *>(node)));
    }
    return valueUInt;
}

/****************************************************************************/

static void parseKeyboard(const xmlNode * keyboard, LayoutDescription::key_list & keys)
{
    char * strEnd;
    unsigned kbX = parseUInt(keyboard, KEYBOARD_ATTR_X, 10);
    unsigned kbY = parseUInt(keyboard, KEYBOARD_ATTR_Y, 10);
    unsigned kbWidth = parseUInt(keyboard, KEYBOARD_ATTR_WIDTH, 10);
    unsigned kbHeight = parseUInt(keyboard, KEYBOARD_ATTR_HEIGHT, 10);
    LayoutDescription::Key::block_type kbZone = parseUInt(keyboard, KEYBOARD_ATTR_ZONE, 0);

    unsigned nbRows = 0;
    for (const xmlNode * row = keyboard->children; row != nullptr; row = row->next) {
        if (row->type == XML_ELEMENT_NODE || xmlStrcmp(row->name, ROW_TAG) == 0) { nbRows += 1; }
    }

    unsigned rowIdx = 0;
    for (const xmlNode * row = keyboard->children; row != nullptr; row = row->next) {
        if (row->type != XML_ELEMENT_NODE || xmlStrcmp(row->name, ROW_TAG) != 0) { continue; }

        unsigned totalWidth = 0;
        for (const xmlNode * key = row->children; key != nullptr; key = key->next) {
            if (key->type != XML_ELEMENT_NODE || xmlStrcmp(key->name, KEY_TAG) != 0) { continue; }
            xmlString keyWidthStr(xmlGetProp(const_cast<xmlNode *>(key), KEY_ATTR_WIDTH), xmlFree);
            if (keyWidthStr != nullptr) {
                auto keyWidthFloat = ::strtof((char*)keyWidthStr.get(), &strEnd);
                if (*strEnd != '\0') {
                    std::ostringstream errMsg;
                    errMsg <<"Value '" <<keyWidthStr.get() <<"' in attribute '"
                           <<KEY_ATTR_WIDTH <<"' of '" <<KEY_TAG
                           <<"' element cannot be parsed as a float";
                    throw LayoutDescription::ParseError(errMsg.str(), xmlGetLineNo(const_cast<xmlNode *>(key)));
                }
                totalWidth += (unsigned int)(keyWidthFloat * 1000);
            } else {
                totalWidth += 1000;
            }
        }

        unsigned xOffset = 0;
        for (const xmlNode * key = row->children; key != nullptr; key = key->next) {
            if (key->type != XML_ELEMENT_NODE || xmlStrcmp(key->name, KEY_TAG) != 0) { continue; }
            xmlString code(xmlGetProp(const_cast<xmlNode *>(key), KEY_ATTR_CODE), xmlFree);
            xmlString glyph(xmlGetProp(const_cast<xmlNode *>(key), KEY_ATTR_GLYPH), xmlFree);
            xmlString keyWidthStr(xmlGetProp(const_cast<xmlNode *>(key), KEY_ATTR_WIDTH), xmlFree);

            unsigned keyWidth;
            if (keyWidthStr != nullptr) {
                keyWidth = kbWidth
                         * (unsigned int)(::strtof((char*)keyWidthStr.get(), &strEnd) * 1000)
                         / totalWidth;
            } else {
                keyWidth = kbWidth * 1000 / totalWidth;
            }

            if (code != nullptr) {
                auto codeVal = LayoutDescription::Key::code_type(parseUInt(key, KEY_ATTR_CODE, 0));
                auto codeNameStr = glyph != nullptr ? std::string(reinterpret_cast<char *>(glyph.get()))
                                                    : std::string();
                std::transform(codeNameStr.begin(), codeNameStr.end(), codeNameStr.begin(), ::toupper);

                keys.emplace_back(
                    kbZone,
                    codeVal,
                    LayoutDescription::Rect {
                        kbX + xOffset,
                        kbY + rowIdx * (kbHeight / nbRows),
                        kbX + xOffset + keyWidth - 1,
                        kbY + (rowIdx + 1) * (kbHeight / nbRows) - 1
                    },
                    codeNameStr
                );
            }
            xOffset += keyWidth;
        }
        rowIdx += 1;
    }
}

/****************************************************************************/

LayoutDescription::LayoutDescription(std::string name, key_list keys)
 : m_name(std::move(name)),
   m_keys(std::move(keys))
{
}

LayoutDescription LayoutDescription::parse(std::istream & stream)
{
    // Parser context
    xmlParserCtxt_ptr context(xmlNewParserCtxt(), xmlFreeParserCtxt);
    if (context == nullptr) {
        throw std::runtime_error("Failed to initialize libxml");
    }
    xmlSetStructuredErrorFunc(context.get(), hideErrorFunc);

    // Document
    std::ostringstream bufferStream;
    bufferStream << stream.rdbuf();
    std::string buffer = bufferStream.str();
    xmlDoc_ptr document(
        xmlCtxtReadMemory(context.get(), buffer.data(), buffer.size(),
                          nullptr, nullptr, XML_PARSE_NOWARNING | XML_PARSE_NONET),
        xmlFreeDoc
    );
    if (document == nullptr) {
        auto error = xmlCtxtGetLastError(context.get());
        if (error == nullptr) { throw ParseError("empty file", 1); }
        std::string errMsg(error->message);
        errMsg.erase(errMsg.find_last_not_of(" \r\n") + 1);
        throw ParseError(errMsg, error->line);
    }

    // Search for keyboard nodes
    const xmlNode * const root = xmlDocGetRootElement(document.get());
    auto name = xmlString(xmlGetProp(const_cast<xmlNode *>(root), ROOT_ATTR_NAME), xmlFree);

    key_list keys;
    for (const xmlNode * node = root->children; node != nullptr; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, KEYBOARD_TAG) == 0) {
            parseKeyboard(node, keys);
        }
    }

    // Finalize
    return LayoutDescription(
        std::string(reinterpret_cast<std::string::const_pointer>(name.get())),
        std::move(keys)
    );
}

/****************************************************************************/

LayoutDescription::Key::Key(block_type block, code_type code, Rect position, std::string name)
 : block(block), code(code), position(position), name(std::move(name))
{}
