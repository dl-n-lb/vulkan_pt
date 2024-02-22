CC = gcc
VMA_LOCATION = "./VulkanMemoryAllocator/include"
CIMGUI_INCLUDE = ./cimgui/
IMGUI_INCLUDE = ./cimgui/imgui
IMGUI_BACKEND_INCLUDE = ./cimgui/imgui/backends
CIMGUI_OBJS = ./cimgui/cimgui.o
CFLAGS = `sdl2-config --cflags` -I$(VMA_LOCATION) -I$(CIMGUI_INCLUDE)
CFLAGS += -I$(IMGUI_BACKEND_INCLUDE) -O2
LIBS = `sdl2-config --libs` -lvulkan -lstdc++ -lm -L./cimgui -l:./cimgui.so

CXXFLAGS = `sdl2-config --cflags` -I$(IMGUI_INCLUDE) -O2 -fno-exceptions -fno-rtti "-DIMGUI_IMPL_API=extern \"C\""

IMGUI_BACKEND_OBJS = cimgui/imgui/backends/imgui_impl_sdl2.o cimgui/imgui/backends/imgui_impl_vulkan.o

shaders/ray_gen.spv: shaders/miss.rmiss
	glslc -o shaders/ray_gen.spv shaders/ray_gen.rgen --target-spv=spv1.6
	glslc -o shaders/closest_hit.spv shaders/closest_hit.rchit --target-spv=spv1.6
	glslc -o shaders/miss.spv shaders/miss.rmiss --target-spv=spv1.6
	glslc -o shaders/shadow_rmiss.spv shaders/shadow_rmiss.rmiss --target-spv=spv1.6


vk_mem_alloc.a: vk_mem_alloc.cpp
	$(CXX) -o vk_mem_alloc.o vk_mem_alloc.cpp -c $(CFLAGS) $(LIBS)
	ar rvs vk_mem_alloc.a vk_mem_alloc.o

main: main.c vk_mem_alloc.a shaders/ray_gen.spv  $(IMGUI_BACKEND_OBJS)
	$(CC) -o main main.c vk_mem_alloc.a $(IMGUI_BACKEND_OBJS) $(CFLAGS) $(LIBS)
