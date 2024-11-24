#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <TextureReader.h>
#include <glad/glad.h>

TEST_CASE("TextureReader initializes with default values") {
    JLEngine::TextureReader reader;

    REQUIRE(reader.GetWidth() == 0);
    REQUIRE(reader.GetHeight() == 0);
    REQUIRE(reader.GetFormat() == 0);
    REQUIRE(reader.GetInternalFormat() == 0);
}

TEST_CASE("TextureReader loads grayscale texture") {
    JLEngine::TextureReader reader;
    bool success = reader.ReadTexture("TestFiles/heightmap.png");

    REQUIRE(success == true);
    REQUIRE(reader.GetWidth() > 0);
    REQUIRE(reader.GetHeight() > 0);
    REQUIRE(reader.GetFormat() == GL_RED);
}

TEST_CASE("TextureReader handles unsupported formats gracefully") {
    JLEngine::TextureReader reader;
    bool success = reader.ReadTexture("TestFiles/terrain.raw");

    REQUIRE(success == false);
    REQUIRE(reader.GetWidth() == 0);
    REQUIRE(reader.GetHeight() == 0);
    REQUIRE(reader.GetFormat() == 0);
}

SCENARIO("Loading a valid texture file") {
    GIVEN("A TextureReader instance and a valid texture file") {
        JLEngine::TextureReader reader;
        std::string validTexturePath = "TestFiles/testpng_256x256.png";

        WHEN("ReadTexture is called with a valid file path") {
            bool success = reader.ReadTexture(validTexturePath);

            THEN("The operation should succeed") {
                REQUIRE(success == true);
            }

            THEN("The width, height, and format should be set correctly") {
                REQUIRE(reader.GetWidth() > 0);
                REQUIRE(reader.GetHeight() > 0);
                REQUIRE((reader.GetFormat() == GL_RGB || reader.GetFormat() == GL_RGBA));
            }
        }
    }
}

SCENARIO("Loading an invalid texture file") {
    GIVEN("A TextureReader instance and an invalid texture file") {
        JLEngine::TextureReader reader;
        std::string invalidTexturePath = "TestFiles/invalidtexture.png";

        WHEN("ReadTexture is called with an invalid file path") {
            bool success = reader.ReadTexture(invalidTexturePath);

            THEN("The operation should fail") {
                REQUIRE(success == false);
            }

            THEN("The width, height, and format should not be set") {
                REQUIRE(reader.GetWidth() == 0);
                REQUIRE(reader.GetHeight() == 0);
                REQUIRE(reader.GetFormat() == 0);
            }
        }
    }
}

SCENARIO("Clearing the TextureReader state") {
    GIVEN("A TextureReader instance that has loaded a texture") {
        JLEngine::TextureReader reader;
        reader.ReadTexture("test_textures/valid_texture.png");

        WHEN("Clear is called") {
            reader.Clear();

            THEN("The width, height, format, and internal format should be reset") {
                REQUIRE(reader.GetWidth() == -1);
                REQUIRE(reader.GetHeight() == -1);
                REQUIRE(reader.GetFormat() == -1);
                REQUIRE(reader.GetInternalFormat() == -1);
            }
        }
    }
}