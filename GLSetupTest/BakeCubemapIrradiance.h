#ifndef TEST_2_H
#define TEST_2_H

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>     
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "JLEngine.h"
#include "TextureReader.h"
#include "TextureWriter.h"
#include "CubemapBaker.h"
#include "Geometry.h"
#include "CubemapBaker.h"
#include "ImageData.h"

int BakeCubemapIrradiance(std::string assetFolder);

#endif