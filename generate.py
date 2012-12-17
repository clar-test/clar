#!/usr/bin/env python

from __future__ import with_statement
from string import Template
import re, fnmatch, os, codecs, pickle

class Module(object):
    class Template(object):
        def __init__(self, module):
            self.module = module

        def _render_callback(self, cb):
            if not cb:
                return '{ NULL, NULL }'
            return '{ "%s", &%s }' % (cb['short_name'], cb['symbol'])

    class DeclarationTemplate(Template):
        def render(self):
            return "\n".join("extern %s;" % cb['declaration'] for cb in self.module.callbacks)

    class CallbacksTemplate(Template):
        def render(self):
            out = "static const struct clar_func _clar_cb_%s[] = {" % self.module.name
            out += ",".join(self._render_callback(cb) for cb in self.module.callbacks)
            out += "};"
            return out

    class InfoTemplate(Template):
        def render(self, index):
            return Template(
            r"""{
                    ${suite_index},
                    "${clean_name}",
                    ${initialize},
                    ${cleanup},
                    ${categories},
                    ${cb_ptr}, ${cb_count}
                }"""
            ).substitute(
                suite_index = index,
                clean_name = self.module.clean_name(),
                initialize = self._render_callback(self.module.initialize),
                cleanup = self._render_callback(self.module.cleanup),
                categories = "NULL", # TODO
                cb_ptr = "&_clar_cb_%s" % self.module.name,
                cb_count = len(self.module.callbacks)
            )

    def __init__(self, name):
        self.mtime = None
        self.size = None
        self.name = name
        self.index = 0

    def clean_name(self):
        return self.name.replace("_", "::")

    def _skip_comments(self, text):
        SKIP_COMMENTS_REGEX = re.compile(
            r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
            re.DOTALL | re.MULTILINE)

        def _replacer(match):
            s = match.group(0)
            return "" if s.startswith('/') else s

        return re.sub(SKIP_COMMENTS_REGEX, _replacer, text)

    def parse(self, contents):
        TEST_FUNC_REGEX = r"^(void\s+(test_%s__(\w+))\(\s*void\s*\))\s*\{"

        contents = self._skip_comments(contents)
        regex = re.compile(TEST_FUNC_REGEX % self.name, re.MULTILINE)

        self.callbacks = []
        self.initialize = None
        self.cleanup = None

        for (declaration, symbol, short_name) in regex.findall(contents):
            data = {
                "short_name" : short_name,
                "declaration" : declaration,
                "symbol" : symbol
            }

            if short_name == 'initialize':
                self.initialize = data
            elif short_name == 'cleanup':
                self.cleanup = data
            else:
                self.callbacks.append(data)

        print "Loaded %d callbacks" % len(self.callbacks)
        return self.callbacks != []

    def refresh(self, path):
        try:
            st = os.stat(path)
            if st.st_mtime == self.mtime and st.st_size == self.size:
                print "File %s has not changed" % path
                return True

            self.mtime = st.st_mtime
            self.size = st.st_size

            with open(path) as fp:
                raw_content = fp.read()

        except IOError:
            return False

        print "Refreshing file %s (%d bytes)" % (path, len(raw_content))
        return self.parse(raw_content)

class TestSuite(object):
    def find_modules(self, path):
        modules = []
        for root, _, files in os.walk(path):
            module_root = root[len(path):]
            module_root = [c for c in module_root.split(os.sep) if c]

            tests_in_module = fnmatch.filter(files, "*.c")

            for test_file in tests_in_module:
                full_path = os.path.join(root, test_file)
                module_name = "_".join(module_root + [test_file[:-2]])

                modules.append((full_path, module_name))

        return modules

    def load_cache(self, path):
        path = os.path.join(path, '.clarcache')
        cache = {}

        # TODO: remove
        return {}

        try:
            fp = open(path)
            cache = pickle.load(fp)
            fp.close()
        except IOError:
            pass

        return cache

    def save_cache(self, path):
        path = os.path.join(path, '.clarcache')
        with open(path, 'w') as cache:
            pickle.dump(self.modules, cache)

    def load(self, path):
        module_data = self.find_modules(path)
        self.modules = self.load_cache(path)

        for path, name in module_data:
            if name not in self.modules:
                self.modules[name] = Module(name)

            if not self.modules[name].refresh(path):
                del self.modules[name]

    def render(self, path):
        path = os.path.join(path, 'clar.suite')
        with open(path, 'w') as data:
            for module in self.modules.values():
                t = Module.DeclarationTemplate(module)
                data.write(t.render())

            for module in self.modules.values():
                t = Module.CallbacksTemplate(module)
                data.write(t.render())

            suites = "static const struct clar_suite _clar_suites[] = {" + ','.join(
                Module.InfoTemplate(module).render(i)
                for i, module in enumerate(self.modules.values())
            ) + "};"

            data.write(suites)

            data.write("static size_t _clar_suite_count = %d;" % self.suite_count())
            data.write("static size_t _clar_callback_count = %d;" % self.callback_count())

suite = TestSuite()

suite.load('.')
print "Loaded %d suites" % len(suite.modules)
suite.render('.')
suite.save_cache('.')
