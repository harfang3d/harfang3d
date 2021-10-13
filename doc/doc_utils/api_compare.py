import argparse
import xml.etree.ElementTree as ETree
import json

def publish_updates(parent, uid, name, status):
    for key, value in status.items():
        if value:
            if uid not in parent:
                parent[uid] = { name : { key: value } }
            elif name not in parent[uid]:
                parent[uid][name] = { key: value}
            else:    
                parent[uid][name][key] = value

def compare_attributes(parent, uid, e0, e1, attributes):
    status = {
        'removed': [],
        'updated': [],
        'added': []
    }

    for name in attributes:
        attr0 = e0.get(name)
        attr1 = e1.get(name)
        if attr0 != attr1:
            key = 'updated'
            if attr0 is None:
                key = 'added'
            elif attr1 is None:
                key = 'removed'
            status[key].append(name)

    publish_updates(parent, uid, 'attributes', status)

def compare_element(status, source, target, tag, callback=None):
    for it in target.findall(tag):
        uid = it.get('uid')
        if source.find(".//%s[@uid='%s']" % (tag, uid)) is None:
            status['added'].append(uid)

    for it in source.findall(tag):
        uid = it.get('uid')
        other = target.find(".//%s[@uid='%s']" % (tag, uid))
        if other is None:
            status['removed'].append(uid)
        elif callback is not None:
            callback(it, other)

def compare_enum(status, e0, e1):
    for it in e0.findall('entry'):
        name = it.get('name')
        other = e1.find(".//entry[@name='%s']" % name)
        if other is None:
            status['removed'].append(name)

    for it in e1.findall('entry'):
        name = it.get('name')
        other = e0.find(".//entry[@name='%s']" % name)
        if other is None:
            status['added'].append(name)

def compare_children(parent, uid, e0, e1, child, tag, attributes=None):
    status = {
        'removed': [],
        'updated': {},
        'added': []
    }

    for it in e0.findall(child):
        name = it.get(tag)
        other = e1.find(".//%s[@%s='%s']" % (child, tag, name))
        if other is None:
            status['removed'].append(name)
        elif attributes is not None:
            compare_attributes(status['updated'], name, it, other, attributes)

    for it in e1.findall(child):
        name = it.get(tag)
        other = e0.find(".//%s[@%s='%s']" % (child, tag, name))
        if other is None:
            status['added'].append(name)

    publish_updates(parent, uid, child, status)
   
def compare_function(status, f0, f1):
    uid = f0.get('uid')
    compare_attributes(status['updated'], uid, f0, f1, ['returns', 'global', 'static'])
    compare_children(status['updated'], uid, f0, f1, 'parm', 'name', ['type'])

def compare_class(status, c0, c1):
    uid = c0.get('uid')
    compare_element(status, c0, c1, 'function', lambda src, target : compare_function(status, src, target))
    compare_children(status['updated'], uid, c0, c1, 'variable', 'name', ['type', 'static'])
    compare_children(status['updated'], uid, c0, c1, 'inherits', 'uid')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('previous', help='filename of the previous version of the API XML description')
    parser.add_argument('current', help='filename of the current version of the API XML description')
    parser.add_argument('--out', required=False, help='output file', default='diff.json')
    args = vars(parser.parse_args())

    status = {
        'removed': [],
        'updated': {},
        'added': []
    }

    previous = ETree.parse(args['previous']).getroot()
    current = ETree.parse(args['current']).getroot()

    compare_element(status, previous, current, 'class', lambda source, target : compare_class(status, source, target))
    compare_element(status, previous, current, 'function', lambda source, target : compare_function(status, source, target))
    compare_element(status, previous, current, 'enum', lambda source, target: compare_enum(status, source, target))

    with open(args['out'], 'w') as out:
        json.dump(status, out, sort_keys=True, indent=4)
