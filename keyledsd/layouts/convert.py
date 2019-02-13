import argparse
import sys
import xml.etree.ElementTree as etree
from collections import namedtuple
from datetime import datetime


class ParseError(Exception): pass


class Parser:
    Key = namedtuple('Key', 'code x y width height glyph')
    Keyboard = namedtuple('Keyboard', 'zone keys')
    Spurious = namedtuple('Spurious', 'zone code')

    def __init__(self, fd):
        try:
            self.tree = etree.parse(fd)
        except etree.ParseError as exc:
            raise ParseError(str(exc))
        self.root = self.tree.getroot()
        if self.root.tag != 'keyboards':
            raise ParseError('root is not a <keyboards> element')

    @property
    def layout(self):
        return self.root.attrib.get('layout')

    @property
    def spurious(self):
        try:
            value = self._spurious
        except AttributeError:
            value = self._spurious = tuple(
                self.Spurious(zone=int(elt.attrib['zone'], base=0),
                              code=int(elt.attrib['code'], base=0))
                for elt in self.root if elt.tag == 'spurious'
            )
        return value

    @property
    def keyboards(self):
        try:
            value = self._keyboards
        except AttributeError:
            value = self._keyboards = tuple(
                self._parse_keyboard(elt) for elt in self.root if elt.tag == 'keyboard'
            )
        return value

    def _parse_keyboard(self, node):
        offsetX = int(node.attrib['x'])
        offsetY = int(node.attrib['y'])
        width = int(node.attrib['width'])
        height = int(node.attrib['height'])
        zone = int(node.attrib.get('zone', 1))
        keys = []

        rownodes = tuple(item for item in node if item.tag == 'row')
        for rowidx, rownode in enumerate(rownodes):
            keynodes = tuple(item for item in rownode if item.tag == 'key')

            totalwidth = sum(float(keynode.attrib.get('width', 1.0)) for keynode in keynodes)

            offset = 0
            for keynode in keynodes:
                keywidth = float(keynode.attrib.get('width', 1.0))

                if 'code' in keynode.attrib:
                    keys.append(self.Key(
                        code=int(keynode.attrib['code'], base=0),
                        x=int(offsetX + offset * width / totalwidth),
                        y=int(offsetY + rowidx * height / len(rownodes)),
                        width=int(keywidth * width / totalwidth),
                        height=int(height / len(rownodes)),
                        glyph=keynode.attrib.get('glyph')
                    ))
                offset += keywidth

        return self.Keyboard(zone=zone, keys=tuple(keys))


def main(infile, outfile):
    try:
        parser = Parser(infile)
    except ParseError as exc:
        print('Parse error: {}'.format(exc), file=sys.stderr)
        return 1


    outfile.write('# Keyleds layout file\n'
                  '# Generated from {name} on {timestamp:%Y-%m-%d %H:%M:%S}\n'
                  '---\n\n'.format(name=infile.name, timestamp=datetime.now()))
    outfile.write('layout: {}\n'.format(parser.layout))

    if parser.spurious:
        outfile.write('spurious:\n')
        outfile.write('\n'.join(
            '    - {{zone: {zone}, code: {code:#04x}}}\n'.format(**item._asdict())
            for item in parser.spurious
        ))

    outfile.write('keyboards:\n')
    for keyboard in parser.keyboards:
        outfile.write('    - ')
        if keyboard.zone != 1:
            outfile.write('zone: {zone}\n      '.format(zone=keyboard.zone))
        outfile.write('keys:\n')
        for key in keyboard.keys:
            outfile.write('        - {')
            outfile.write('code: {code:#04x}, x: {x}, y: {y}, width: {width}, height: {height}'.format(
                          **key._asdict()))
            if key.glyph:
                outfile.write(', glyph: {!r}'.format(key.glyph))
            outfile.write('}\n')

    return 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert XML layout to keyleds YAML layout')
    parser.add_argument('infile', nargs='?', type=argparse.FileType('rb'), default=sys.stdin)
    parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'), default=sys.stdout)
    args = parser.parse_args()
    sys.exit(main(**vars(args)))
