#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>
#include <glm/glm.hpp>
#include <string>

#include "TextureProgram.hpp"
#include "GL.hpp"

struct PosTexVertex {
    glm::vec3 Position;
    glm::vec2 TexCoord;
};

static_assert( sizeof(PosTexVertex) == 3*4 + 2*4, "PosTexVertex is the size we expect it to be");

struct tex_struct {
    // handles
    GLuint tex = 0;
    GLuint tristrip = 0; 					 //buffer
    GLuint tristrip_for_texture_program = 0; //vao

    GLsizei count = 0;
    glm::mat4 CLIP_FROM_LOCAL = glm::mat4(1.f);

    hb_position_t end_pos_x = 0;
    hb_position_t end_pos_y = 0;
};

int test_freetype( FT_GlyphSlot *in_glyph, hb_codepoint_t glyphid );

std::vector<tex_struct> test_harfbuzz ( const char *text, hb_position_t start_pos_x, hb_position_t start_pos_y);

tex_struct init_glyph_tex (hb_codepoint_t glyphid, float x_pos, float y_pos);
tex_struct init_image_tex (std::string image_path);
void draw_glyph (tex_struct glyph_tex);

std::vector<glm::u8vec4> load_bitmap(FT_Bitmap bitmap, glm::uvec2 &size);