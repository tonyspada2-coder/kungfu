from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout

class YijinjingSHMConan(ConanFile):
    name = "yijinjing_shm"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("fmt/8.1.1")
        self.requires("nlohmann_json/3.11.2")
        self.requires("nng/1.5.2")
        self.requires("rxcpp/4.1.1")
        self.requires("spdlog/1.10.0")
        self.requires("tabulate/1.4")
        self.requires("catch2/2.13.8")
        self.requires("sqlite3/3.45.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def configure(self):
        self.options["fmt"].header_only = True
        self.options["spdlog"].header_only = True
        self.options["nng"].http = False
