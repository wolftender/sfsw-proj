#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <vector>

#include <glad/glad.h>

namespace mini {
    enum CUBEMAP_SIDE {
        CUBEMAP_SIDE_RIGHT  = 0,
        CUBEMAP_SIDE_LEFT   = 1,
        CUBEMAP_SIDE_TOP    = 2,
        CUBEMAP_SIDE_BOTTOM = 3,
        CUBEMAP_SIDE_FRONT  = 4,
        CUBEMAP_SIDE_BACK   = 5
    };

    struct cubemap_side_info {
        uint32_t width, height;
        GLenum format;
        unsigned char* data;
    };

    struct cubemap_info {
        std::array<cubemap_side_info, 6> sides;
    };

    class cubemap {
        private:
            GLuint m_handle;

        public:
            cubemap(const cubemap_info& info);
            ~cubemap();

            cubemap(const cubemap&) = delete;
            cubemap& operator=(const cubemap&) = delete;

            void bind(GLenum slot = GL_TEXTURE0) const;

        public:
            static std::shared_ptr<cubemap> load_from_files(
                    const std::array<std::string, 6>& sides);
    };
}