#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "textstuff.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	
	//camera:
	Scene::Camera *camera = nullptr;

	struct Room {
		std::vector<const char *> descriptions;
		std::vector<std::string> image_paths;
		std::vector<uint8_t> neighbor_idxs;
		int8_t translates = -1;
	};

	const char *unknown = "???";
	std::vector<std::vector<const char *>> translated_words = {
		{unknown, "barrel", "water", "secret other liquid"}, 
		{unknown}, 
		{unknown},
		{unknown},
		{unknown},
		{unknown},
		{unknown},
		{unknown}
	};
	std::vector<uint8_t> translation_idxs = {0, 0, 0, 0, 0, 0, 0, 0};

	std::vector<Room> all_rooms;

	uint8_t active_room_idx = 0;

	const char *message = "This is a message. Wow no newLineS oWo";

	// texture stuff

	std::vector<tex_struct> glyph_textures;
	std::vector<tex_struct> image_textures;
};
