from conan import ConanFile
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.files import copy
import os

class FossilThreadsConan(ConanFile):
    name = "fossil_threads"
    version = "0.1.1"
    license = "MPL-2.0"
    author = "Fossil Logic <michaelbrockus@gmail.com>"
    url = "https://github.com/fossillogic/fossil-threads"
    description = "Fossil Threads is a lightweight, portable multithreading library written in pure C with no external dependencies."
    topics = ("c", "thread", "mutex", "condition", "cpp", "meson", "conan-recipe", "mesonbuild", "ninja-build")

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    exports_sources = "code/**", "meson.build", "meson.options"
    generators = "PkgConfigDeps"

    def layout(self):
        """Define a basic build/source folder layout"""
        self.folders.source = "."
        self.folders.build = "builddir"

    def generate(self):
        """Generate Meson toolchain files"""
        tc = MesonToolchain(self)
        tc.generate()

    def build(self):
        """Configure and build the project using Meson"""
        meson = Meson(self)
        meson.configure()
        meson.build()

    def source(self):
        self.run(f"git clone --branch v{self.version} --depth 1 {self.url}")

    def package(self):
        """Install headers and libraries into package folder"""
        meson = Meson(self)
        meson.install()

        # Ensure headers are included even if not installed by Meson
        copy(self, "*.h",
             src="code/logic/fossil/threads",
             dst=os.path.join(self.package_folder, "include", "fossil", "threads"))

    def package_info(self):
        """Set information for consumers of the package"""
        self.cpp_info.libs = ["fossil_threads"]
        self.cpp_info.includedirs = ["include"]

    def source(self):
        self.run(f"git clone --branch v{self.version} {self.url}")