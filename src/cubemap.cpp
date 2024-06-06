#include <fstream>
#include <lodepng.h>

#include "cubemap.hpp"

namespace mini {
    struct side_image_source {
        std::vector<unsigned char> data;
        unsigned int width, height;
    };

    cubemap::cubemap(const cubemap_info &info) : m_handle(0) {
        GLuint handle = 0;
        glGenTextures(1, &handle);

        glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
        for (auto i = 0UL; i < info.sides.size(); ++i) {
            const auto& side = info.sides[i];
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, side.format, side.width, side.height, 
                0, side.format, GL_UNSIGNED_BYTE, side.data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        m_handle = handle;
    }

    cubemap::~cubemap() {
        if (m_handle) {
            glDeleteTextures(1, &m_handle);
        }
    }

    void cubemap::bind(GLenum slot) const {
        if (m_handle) {
            glActiveTexture(slot);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
        }
    }

    std::shared_ptr<cubemap> cubemap::load_from_files(const std::array<std::string, 6> &sides) {
        std::array<side_image_source, 6> side_images;
        cubemap_info info;

        for (auto side = 0UL; side < sides.size(); ++side) {
            const auto& path = sides[side]; 
            std::fstream stream(path, std::ios::binary | std::ios::in);

            if (!stream) {
			    throw std::runtime_error ("cannot open file: " + path);
		    }

            stream.seekg(0, std::ios::end);
            uint64_t size = stream.tellg();
            stream.seekg(0, std::ios::beg);

            std::vector<uint8_t> buffer;
            buffer.resize(size);

            stream.read(reinterpret_cast<char *>(buffer.data()), size);
            stream.close();

            // decode image
            unsigned int error = lodepng::decode(
                side_images[side].data, 
                side_images[side].width, 
                side_images[side].height, 
                buffer);

            if (error != 0) { // failed to load image
			    throw std::runtime_error("failed to load texture: " + 
                    std::string(lodepng_error_text (error)));
		    }

            info.sides[side].data = side_images[side].data.data();
            info.sides[side].width = side_images[side].width;
            info.sides[side].height = side_images[side].height;
            info.sides[side].format = GL_RGBA;
        }

        return std::make_shared<cubemap>(info);
    }
}