#include "textstuff.hpp"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

// adapted from freetype tutorial online:
// https://freetype.org/freetype2/docs/tutorial

int test_freetype( FT_GlyphSlot *in_glyph, hb_codepoint_t glyphid) {
    FT_Library library;
    FT_Face face;

    FT_Error error = FT_Init_FreeType( &library );
    if ( error ) {
        std::cout << "an error occured during FT initialization" << std::endl;
        return 1;
    }
    error = FT_New_Face( library,
                     data_path("Fanwood_Text/FanwoodText-Regular.ttf").c_str(),
                     0,
                     &face );
    if ( error == FT_Err_Unknown_File_Format )
    {
        std::cout << "the font file could be opened and read, " <<
            "but it appears that its font format is unsupported." << std::endl;
        return 1;
    }
    else if ( error )
    {
        std::cout << "another error code means that the font file could not " <<
            "be opened or read, or that it is broken." << std::endl;
        return 1;
    }

    error = FT_Set_Char_Size(
          face,    /* handle to face object         */
          0,       /* char_width in 1/64 of points  */
          16*64,   /* char_height in 1/64 of points */
          300,     /* horizontal device resolution  */
          300 );   /* vertical device resolution    */
    
    FT_UInt glyph_index = FT_Get_Char_Index( face, glyphid + 0x1d );

    error = FT_Load_Glyph(
          face,          /* handle to face object */
          glyph_index,   /* glyph index           */
          FT_LOAD_DEFAULT );  /* load flags, see below */
    
    error = FT_Render_Glyph( face->glyph,   /* glyph slot  */
                         FT_RENDER_MODE_NORMAL ); /* render mode */

    *in_glyph = face->glyph;
    return 0;
}

// small harfbuzz example. adapted from the online documentation: 
// https://harfbuzz.github.io/a-simple-shaping-example.html

std::vector<tex_struct> test_harfbuzz (const char *text, hb_position_t start_pos_x, hb_position_t end_pos_x) {
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text, -1, 0, -1);

    // direction, script, lang
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    // get font and face
    hb_blob_t *blob = hb_blob_create_from_file(data_path("Fanwood_Text/FanwoodText-Regular.ttf").c_str());
        /* or hb_blob_create_from_file_or_fail() */
    hb_face_t *face = hb_face_create(blob, 0);
    hb_font_t *font = hb_font_create(face);

    // shape!
    hb_shape(font, buf, NULL, 0);

    // get glyph and position info
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    // Iterate over each glyph.
    hb_position_t cursor_x = start_pos_x;
    hb_position_t cursor_y = end_pos_x;
    std::vector<tex_struct> ret;
    // init_glyph_tex(glyph_info[0].codepoint, cursor_x, cursor_y);
    // ret.resize(glyph_count);
    for (unsigned int i = 0; i < glyph_count; i++) {
        hb_codepoint_t glyphid  = glyph_info[i].codepoint;
        hb_position_t x_offset  = glyph_pos[i].x_offset;
        hb_position_t y_offset  = glyph_pos[i].y_offset;
        hb_position_t x_advance = glyph_pos[i].x_advance;
        hb_position_t y_advance = glyph_pos[i].y_advance;
        // printf("id of glyph %u: %u\n", i, glyphid);
        if (glyphid == 0x0) {
            // newline
            cursor_y -= 1000. * 64. * 0.08;
            cursor_x = 0;
        }
        // printf("x_pos: %f, y_pos: %f\nx_advance:%f, y_advance:%f\n", 
        //                             (float) (cursor_x + x_offset) / 64., 
        //                             (float) (cursor_y + y_offset) / 64.,
        //                             (float) x_advance / 64.,
        //                             (float) y_advance / 64.);
        else {
            tex_struct new_glyph = init_glyph_tex(glyphid, 
                                    (float) (cursor_x + x_offset * 0.8) / 64.f, 
                                    (float) (cursor_y + y_offset) / 64.f);// y_advance / 64.);
            cursor_x += x_advance * 0.8;
            cursor_y += y_advance;
            new_glyph.end_pos_x = cursor_x;
            new_glyph.end_pos_y = cursor_y;
            ret.emplace_back(new_glyph);
        }
    }
    
    // cleanup
    hb_buffer_destroy(buf);
    hb_font_destroy(font);
    hb_face_destroy(face);
    hb_blob_destroy(blob);
    return ret;
};

// code in init_glyph_tex adapted from the OpenGL texture rendering lesson in class.
tex_struct init_glyph_tex (hb_codepoint_t glyphid, float x_pos, float y_pos) {
    tex_struct glyph_tex;
    FT_GlyphSlot *glyph_ptr = (FT_GlyphSlot *) calloc(1, sizeof(FT_GlyphSlot));

    int yay = test_freetype(glyph_ptr, glyphid);
	if (yay) {
        printf("whoopsie doopsie, test_freetype failed\n");
        return glyph_tex;
    }
	FT_GlyphSlot glyph = *glyph_ptr;
	free(glyph_ptr);

    y_pos += (float) glyph->bitmap_top / (float) glyph->bitmap.rows;

    glGenTextures(1, &glyph_tex.tex);
	{ // upload a texture
		glm::uvec2 size;
		std::vector< glm::u8vec4 > data = load_bitmap(glyph->bitmap, size);
        // note: load_png throws on failure
		// load_png(data_path("code.png"), &size, &data, LowerLeftOrigin);

		glBindTexture(GL_TEXTURE_2D, glyph_tex.tex);
		// here, "data()" is the member function that gives a pointer to the first element
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, data.data());
		// i for integer. wrap mode, minification, magnification
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// // nearest neighbor
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// aliasing at far distances. maybe moire patterns
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenBuffers(1, &glyph_tex.tristrip);
	{
		glGenVertexArrays(1, &glyph_tex.tristrip_for_texture_program);
		glBindVertexArray(glyph_tex.tristrip_for_texture_program);
		glBindBuffer(GL_ARRAY_BUFFER, glyph_tex.tristrip);
		glVertexAttribPointer(texture_program->Position_vec4,
			3, // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(PosTexVertex), // stride. readable code win
			(GLbyte *) 0 + offsetof(PosTexVertex, Position) // offset
		);
		glEnableVertexAttribArray(texture_program->Position_vec4);

		// do all of this again for TexCoord!
		glVertexAttribPointer(texture_program->TexCoord_vec2,
			2, // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(PosTexVertex), // stride
			(GLbyte *) 0 + offsetof(PosTexVertex, TexCoord) // offset
		);
		glEnableVertexAttribArray(texture_program->TexCoord_vec2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	std::vector< PosTexVertex > verts;
	// basically make a quad?
    float scale = 1.f / 1000.f;
    // float aspect = (float) glyph->bitmap.width / (float) glyph->bitmap.rows;
    glm::vec2 glyph_dims = glm::vec2(glyph->metrics.width, glyph->metrics.height) / 64.f;
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(0., 0., 0.f),
		.TexCoord = glm::vec2(0.f, 0.f), // comma is fine and might be convenient
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(0., 1., 0.f),
		.TexCoord = glm::vec2(0.f, 1.f),
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(1., 0., 0.f),
		.TexCoord = glm::vec2(1.f, 0.f),
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(1., 1., 0.f),
		.TexCoord = glm::vec2(1.f, 1.f),
	});
	glBindBuffer(GL_ARRAY_BUFFER, glyph_tex.tristrip);
	// "stream" means "modify once, use at most a few times" (from OpenGL docs)
	// other access types are "static" (modified once, used many times) and 
	// "dynamic" (modified and used many times).
	// "draw" is what we do with the data as opposed to "read", maybe "copy"
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(verts[0]),
		verts.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glyph_tex.count = verts.size();
	glyph_tex.CLIP_FROM_LOCAL = glm::scale(
        glm::translate(
            glm::mat4(
                1., 0., 0., 0.,
                0., 1., 0., 0.,
                0., 0., 1., 0.,
                0., 0., 0., 1.
            )
            , glm::vec3((x_pos) * scale - 0.95, (y_pos + (glyph->metrics.horiBearingY / 64.) - glyph_dims.y) * scale + 0.9, 0.))
        , glm::vec3(glyph_dims.x * scale * 0.8, glyph_dims.y * scale, 1.));
	GL_ERRORS();
    return glyph_tex;
};

// code in init_glyph_tex adapted from the OpenGL texture rendering lesson in class.
tex_struct init_image_tex(std::string image_path) {
    tex_struct image_tex;

    glGenTextures(1, &image_tex.tex);
	{ // upload a texture
		glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        // note: load_png throws on failure
		load_png(image_path, &size, &data, LowerLeftOrigin);

		glBindTexture(GL_TEXTURE_2D, image_tex.tex);
		// here, "data()" is the member function that gives a pointer to the first element
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, data.data());
		// i for integer. wrap mode, minification, magnification
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// // nearest neighbor
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// aliasing at far distances. maybe moire patterns
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenBuffers(1, &image_tex.tristrip);
	{
		glGenVertexArrays(1, &image_tex.tristrip_for_texture_program);
		glBindVertexArray(image_tex.tristrip_for_texture_program);
		glBindBuffer(GL_ARRAY_BUFFER, image_tex.tristrip);
		glVertexAttribPointer(texture_program->Position_vec4,
			3, // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(PosTexVertex), // stride. readable code win
			(GLbyte *) 0 + offsetof(PosTexVertex, Position) // offset
		);
		glEnableVertexAttribArray(texture_program->Position_vec4);

		// do all of this again for TexCoord!
		glVertexAttribPointer(texture_program->TexCoord_vec2,
			2, // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(PosTexVertex), // stride
			(GLbyte *) 0 + offsetof(PosTexVertex, TexCoord) // offset
		);
		glEnableVertexAttribArray(texture_program->TexCoord_vec2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	std::vector< PosTexVertex > verts;
	// basically make a quad?
    float scale = 1.f / 1000.f; (void) scale;
    // float aspect = (float) glyph->bitmap.width / (float) glyph->bitmap.rows;
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(0., 0., 0.f),
		.TexCoord = glm::vec2(0.f, 0.f), // comma is fine and might be convenient
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(0., 1., 0.f),
		.TexCoord = glm::vec2(0.f, 1.f),
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(1., 0., 0.f),
		.TexCoord = glm::vec2(1.f, 0.f),
	});
	verts.emplace_back(PosTexVertex{
		.Position = glm::vec3(1., 1., 0.f),
		.TexCoord = glm::vec2(1.f, 1.f),
	});
	glBindBuffer(GL_ARRAY_BUFFER, image_tex.tristrip);
	// "stream" means "modify once, use at most a few times" (from OpenGL docs)
	// other access types are "static" (modified once, used many times) and 
	// "dynamic" (modified and used many times).
	// "draw" is what we do with the data as opposed to "read", maybe "copy"
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(verts[0]),
		verts.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	image_tex.count = verts.size();
	image_tex.CLIP_FROM_LOCAL = glm::scale(
        glm::translate(
            glm::mat4(
                1., 0., 0., 0.,
                0., 1., 0., 0.,
                0., 0., 1., 0.,
                0., 0., 0., 1.
            )
            , glm::vec3(-.9, -.9, 0.))
        , glm::vec3(1., 1., 1.));
	GL_ERRORS();
    return image_tex;
}

// code in draw_glyph adapted from the OpenGL texture rendering lesson in class.
void draw_glyph(tex_struct glyph_tex) {
    // texture example drawing
	// to display transparency
    glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(texture_program->program);

	GL_ERRORS();

	glBindVertexArray(glyph_tex.tristrip_for_texture_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glyph_tex.tex);
	GL_ERRORS();
	glUniformMatrix4fv(texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, 
		glm::value_ptr(glyph_tex.CLIP_FROM_LOCAL));
	GL_ERRORS();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, glyph_tex.count);

	GL_ERRORS();

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
};

std::vector<glm::u8vec4> load_bitmap(FT_Bitmap bitmap, glm::uvec2 &size) {
    size = glm::vec2(bitmap.width, bitmap.rows);
    std::vector<glm::u8vec4> data;
    for (uint row = 0; row < size.y; row++) {
        for (uint col = 0; col < size.x; col++) {
            data.emplace_back(bitmap.buffer[(size.y - row - 1) * size.x + col]);
        }
    }
    return data;
}