#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	// init rooms
	Room intro_room;
	intro_room.descriptions = {
		"The mages must tend to too many emergencies at once, \n so they have left you in charge of preparing the ingredients \nfor the spell that is supposed to heal your friend's injury. \nThis is a dubious decision, as you are not a mage, but apparently, \nthe instructions for doing this are \"straightforward\" and \"doable\", \nor, they would be, but the instructions are written in the mages'\narcane tongue, which you know very little of.\n\nPress 1 to start making the best of this situation..."
	};
	Room kitchen;
	kitchen.descriptions = {
		"You stare at the note that the mages told you contains the spell instructions.\nYou're sure you can reason this out.\n\n\"Place ", 
		translated_words[0][translation_idxs[0]], " in a ", translated_words[1][translation_idxs[1]], 
		". Add ", translated_words[2][translation_idxs[2]] , " ", translated_words[3][translation_idxs[3]], ", ", 
				  translated_words[2][translation_idxs[2]] , " ", translated_words[4][translation_idxs[4]], 
				  ", and some ", translated_words[4][translation_idxs[4]], " to the ", translated_words[0][translation_idxs[0]], 
		".\nPlace the ", translated_words[1][translation_idxs[1]], " onto the ", translated_words[5][translation_idxs[5]],
		". ", translated_words[6][translation_idxs[6]], " the ", translated_words[5][translation_idxs[5]],
		".\"\n\nPress a number key to investigate each purple word."
	};
	kitchen.image_paths.emplace_back(data_path("recipe.png"));
	intro_room.neighbor_idxs.emplace_back(1);

	Room water;
	water.descriptions = {
		"You find this symbol on a label attached to two large barrels.\nIf you open up the lids, you can see water in them...\n...unless it's a secret other clear odorless liquid.\n\nWhat do you think the symbol means?\n1. Barrel\n2. Water\n3. Secret other liquid"
	};
	water.translates = 0;
	kitchen.neighbor_idxs.emplace_back(2);
	water.neighbor_idxs.emplace_back(1);
	water.neighbor_idxs.emplace_back(1);
	water.neighbor_idxs.emplace_back(1);

	all_rooms.emplace_back(intro_room);
	all_rooms.emplace_back(kitchen);
	all_rooms.emplace_back(water);

	// float height = 0.9;
	for (const char *description : intro_room.descriptions) {
		std::vector<tex_struct> curr_textures = test_harfbuzz(description, 0, 0);
		glyph_textures.insert(glyph_textures.end(), curr_textures.begin(), curr_textures.end());
		// height -= 0.08;
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	int8_t next_room_idx = -1;

	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		return true;
	} else if (evt.type == SDL_MOUSEMOTION) {
		return true;
	} else if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_1) {
			next_room_idx = 0;
		} else if (evt.key.keysym.sym == SDLK_2) {
			next_room_idx = 1;
		} else if (evt.key.keysym.sym == SDLK_3) {
			next_room_idx = 2;
		} else if (evt.key.keysym.sym == SDLK_4) {
			next_room_idx = 3;
		} else if (evt.key.keysym.sym == SDLK_5) {
			next_room_idx = 4;
		} else if (evt.key.keysym.sym == SDLK_6) {
			next_room_idx = 5;
		} else if (evt.key.keysym.sym == SDLK_7) {
			next_room_idx = 6;
		} else if (evt.key.keysym.sym == SDLK_8) {
			next_room_idx = 7;
		} 
	}

	Room active_room = all_rooms[active_room_idx];
	if (next_room_idx >= 0 && next_room_idx < active_room.neighbor_idxs.size()) {

		if (active_room.translates >= 0) {
			translation_idxs[active_room.translates] = next_room_idx + 1; 
			printf("translation idx %d: %u\n", active_room.translates, translation_idxs[active_room.translates]);
			all_rooms[1].descriptions = {
				"You stare at the note that the mages told you contains the spell instructions.\nYou're sure you can reason this out.\n\n\"Place ", 
				translated_words[0][translation_idxs[0]], " in a ", translated_words[1][translation_idxs[1]], 
				". Add ", translated_words[2][translation_idxs[2]] , " ", translated_words[3][translation_idxs[3]], ", ", 
						translated_words[2][translation_idxs[2]] , " ", translated_words[4][translation_idxs[4]], 
						", and some ", translated_words[4][translation_idxs[4]], " to the ", translated_words[0][translation_idxs[0]], 
				".\nPlace the ", translated_words[1][translation_idxs[1]], " onto the ", translated_words[5][translation_idxs[5]],
				". ", translated_words[6][translation_idxs[6]], " the ", translated_words[5][translation_idxs[5]],
				".\"\n\nPress a number key to investigate each purple word."
			};
		}

		active_room_idx = active_room.neighbor_idxs[next_room_idx];

		Room active_room = all_rooms[active_room_idx];

		glyph_textures.clear();
		hb_position_t start_pos_x = 0;
		hb_position_t start_pos_y = 0;
		for (uint8_t i = 0; i < active_room.descriptions.size(); i++) {
			const char *desc = active_room.descriptions[i];
			if (glyph_textures.size() > 0) {
				start_pos_y = glyph_textures.rbegin()->end_pos_y;
				start_pos_x = glyph_textures.rbegin()->end_pos_x;
			} else {
				start_pos_x = 0;
				start_pos_y = 0;
			}
			std::vector<tex_struct> curr_textures = test_harfbuzz(desc, start_pos_x, start_pos_y);
			glyph_textures.insert(glyph_textures.end(), curr_textures.begin(), curr_textures.end());
		}

		image_textures.clear();
		for (uint8_t i = 0; i < active_room.image_paths.size(); i++) {
			image_textures.emplace_back(init_image_tex(active_room.image_paths[i]));
		}
	}
	return false;
}

void PlayMode::update(float elapsed) {
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.0f, 0.02f, 0.01f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	// scene.draw(*camera);

	for (int i = 0; i < glyph_textures.size(); i++) {
		draw_glyph(glyph_textures[i]);
	}
	for (int i = 0; i < image_textures.size(); i++) {
		draw_glyph(image_textures[i]);
	}

	GL_ERRORS();
}
