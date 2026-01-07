# =============================================================================
# Cool Makefile for CMake Projects
# =============================================================================
#
# A Makefile that makes configuring and building CMake projects really simple.


# =============================================================================
# Functions
# =============================================================================

define to_lower_case
$(shell echo $(1) | tr '[:upper:]' '[:lower:]')
endef

define find_in_list
$(shell resolved_value=$$(for candidate in $(2); do \
		if [ "$(call to_lower_case,$(1))" = "$$(echo $$candidate | tr '[:upper:]' '[:lower:]')" ]; then \
			echo $$candidate; \
		fi; \
	done); \
	echo "$${resolved_value:-$(1)}";)
endef

define exists_in_list
$(shell resolved_value=$$(for candidate in $(2); do \
		if [ "$(call to_lower_case,$(1))" = "$$(echo $$candidate | tr '[:upper:]' '[:lower:]')" ]; then \
			echo true; \
		fi; \
	done); \
	echo "$${resolved_value:-false}";)
endef

# =============================================================================
# Variables
# =============================================================================

project_name:=?
project_lang:=?
type:=debug
generator:=$(shell if [ -x "$$(which ninja)" ]; then echo "Ninja"; else echo "Unix Makefiles"; fi)
build_path:=$(shell if [ "$(call to_lower_case,$(generator))" = "xcode" ]; then echo "xcode-build"; else echo "cmake-build-$(call to_lower_case,$(type))"; fi)

# =============================================================================
# Constants
# =============================================================================

override cores:=$(shell if [ "$$(uname)" = "Darwin" ]; then sysctl -n hw.ncpu; elif [ "$$(uname)" = "Linux" ]; then nproc --all; else echo 1; fi)
override valid_generators:="Unix Makefiles" "Ninja" "Xcode"
override valid_types:=Release Debug RelWithDebInfo MinSizeRel
override debug_types:=Debug RelWithDebInfo

override cmake_build_type_arg:=$(call find_in_list,$(type),$(valid_types))
override cmake_generator_arg:=$(call find_in_list,$(generator),$(valid_generators))
override is_debug:=$(call exists_in_list,$(type),$(debug_types))

# =============================================================================
# Utility targets
# =============================================================================

# All targets are phony in this Makefile
.PHONY : $(shell egrep "^[A-Za-z0-9_-]+\:([^\=]|$$)" $(lastword $(MAKEFILE_LIST)) | sed -E 's/(.*):.*/\1/g')

# Default target is build
all: build

#: Displays this message
help:
	@echo "Usage:"
	@echo
	@echo "  make type=<build type> generator=<generator> <target>"
	@echo
	@echo "Targets:"
	@egrep -B1 "^[A-Za-z0-9_-]+\:([^\=]|$$)" $(lastword $(MAKEFILE_LIST)) \
		| grep -v -- -- \
		| sed 'N;s/\n/###/' \
		| sed -n 's/^#: \(.*\)###\(.*\):.*/  \2###\1/p' \
		| column -t  -s '###'

#: List available build types
list-types:
	@for type in $(valid_types); do echo $$type; done

#: List available generators
list-generators:
	@for generator in $(valid_generators); do echo $$generator; done;

validate-type:
ifneq ($(cmake_build_type_arg),$(filter $(cmake_build_type_arg),$(valid_types)))
	@echo "Invalid build type: $(cmake_build_type_arg). Valid types are:" >&2;
	@for type in $(valid_types); do echo - $$type >&2; done
	@exit 1;
else ifeq ($(strip $(cmake_build_type_arg)),)
	@echo "Empty build type. Valid types are:" >&2;
	@for type in $(valid_types); do echo - $$type >&2; done
	@exit 1;
else
	@:
endif

validate-generator:
ifneq ("$(cmake_generator_arg)",$(filter "$(cmake_generator_arg)",$(valid_generators)))
	@echo "Unsupported generator: $(cmake_generator_arg). Valid generators are:" >&2;
	@for generator in $(valid_generators); do echo - $$generator >&2; done;
	@exit 1;
else ifeq ($(strip $(cmake_generator_arg)),)
	@echo "Empty CMake generator. Valid generators are:" >&2;
	@for generator in $(valid_generators); do echo - $$generator >&2; done;
	@exit 1;
else
	@:
endif

# =============================================================================
# Build targets
# =============================================================================

#: Delete the build path for the currently selected type and generator
clean: validate-type
	@rm -rf "./$(build_path)";

#: Delete all build paths for all types and generators
clean-all:
	@for generator in $(valid_generators); do \
		for type in $(valid_types); do \
			$(MAKE) generator="$$generator" type=$$type clean; \
		done; \
	done;

#: Initialise an empty CMake project
init:
	@if [ ! -f CMakeLists.txt ]; then \
		project_name=$(project_name); \
		if [ "$$project_name" = "?" ]; then \
			read -p "Enter project name: " project_name; \
		fi; \
		project_lang=$(project_lang); \
		if [ "$$project_lang" = "?" ]; then \
			read -p "Enter project language (c|cpp): " project_lang; \
		fi; \
		project_lang=$$(echo $$project_lang | tr '[:upper:]' '[:lower:]' | sed -E -e 's/^(c\+\+|cxx)$$/cpp/'); \
		if [ "$$project_lang" = "c" ]; then \
			echo "#include \"stdio.h\"\n\nint main() {\n\tprintf(\"%s\", \"hello, world!\");\n\treturn 0;\n}\n" >$$project_name.$$project_lang; \
		elif [ "$$project_lang" = "cpp" ]; then \
			echo "#include <iostream>\n\nint main() {\n\tstd::cout << \"hello, world!\" << std::endl;\n\treturn 0;\n}\n" >$$project_name.$$project_lang; \
		else \
			echo "Unsupported language type" >&2; \
			exit 1; \
		fi; \
		echo "project($$project_name)\n\nadd_executable($$project_name $$project_name.$$project_lang)\n\n" >CMakeLists.txt; \
	fi;

#: Configure the project for the selected type and generator
configure: validate-type validate-generator init
ifeq ($(cmake_generator_arg), Xcode)
	@cmake -S . -B $(build_path) \
		-G "$(cmake_generator_arg)";
else
	@cmake -S . -B $(build_path) \
		-DCMAKE_BUILD_TYPE=$(cmake_build_type_arg) \
		-G "$(cmake_generator_arg)";
endif

#: Build the project for the selected type and generator
build: configure
ifeq ($(cmake_generator_arg), Xcode)
	@cmake --build $(build_path) --config $(cmake_build_type_arg);
else
	@cmake --build $(build_path) -- -j$(cores);
endif

#: Install the project for the selected type and generator
install: build
ifeq ($(cmake_generator_arg), Xcode)
	@cmake --install $(build_path) --config $(cmake_build_type_arg);
else
	@cmake --install $(build_path);
endif

#: Run the application
run: build
	@app_name=$$(cmake -S . -B "$(build_path)" --trace 2>&1 \
					| egrep -i add_executable \
					| sort -r \
					| head -n 1 \
					| sed -E 's/.*add_executable.[[:space:]]*([A-Za-z0-9\\"_\.+-]+)[[:space:]].*/\1/'); \
	if [ "$(call to_lower_case,$(generator))" = "xcode" ]; then \
		cd $(build_path)/$(cmake_build_type_arg) && ./$$app_name; \
	else \
		cd $(build_path) && ./$$app_name; \
	fi
	@echo "\nProcessed finished with exit code $$?"


# =============================================================================
# Makefile debugging targets
# =============================================================================

list-variables:
	@echo $(foreach \
		v, \
		$(.VARIABLES), \
		$(if \
			$(filter file, $(origin $(v))), \
			\\n$(v)=$($(v)), \
			$(if \
				$(filter command line, $(origin $(v))), \
				\\n$(v)=$($(v)), \
				$(if \
					$(filter override, $(origin $(v))), \
					\\n$(v)=$($(v)))))) | egrep -v '^$$' | egrep -v '=\s*$$' | sort

list-targets:
	@egrep "^[A-Za-z0-9_-]+\:([^\=]|$$)" $(lastword $(MAKEFILE_LIST)) | sed -E 's/(.*):.*/\1/g' | sort
