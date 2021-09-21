#include "Mode.hpp"

#include "Scene.hpp"

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

	virtual Scene::Transform *spawn_copy(Scene::Drawable *to_copy, Scene::Transform *parent);

	//Spawning Helper functions
	virtual Scene::Transform* spawn_shot(glm::vec3 position);

	virtual Scene::Transform* spawn_s_asteroid(glm::vec3 position);
	virtual Scene::Transform* spawn_m_asteroid(glm::vec3 position);
	virtual Scene::Transform* spawn_l_asteroid(glm::vec3 position);

	virtual void delete_shot(Scene::Transform *shot_to_delete);
	virtual void delete_asteroid(Scene::Transform* asteroid_to_delete);

	virtual void simulate_physics(float elapsed);

	virtual void end_game();

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//Spawnable Objects
	Scene::Drawable *s_asteroid= nullptr;
	Scene::Drawable *m_asteroid = nullptr;
	Scene::Drawable *l_asteroid = nullptr;
	Scene::Drawable *shot = nullptr;

	//Player object and vars
	Scene::Transform* player = nullptr;
	float playerRadius = 2.0f;


	std::vector<Scene::Transform*> asteroids;
	glm::vec3 asteroidDirection = glm::vec3(0.0f, -1.0f, 0.0f);
	std::vector<float> velocities;
	std::vector<float> radii;

	std::vector<Scene::Transform*> shots;
	glm::vec3 shotDirection = glm::vec3(0.0f, 1.0f, 0.0f);
	float shotVelocity = 20.0f;
	float shotRadius = .3f;

	glm::vec3 shotOffsetFromPlayer = glm::vec3(0.0f, 7.0f, 0.0f);

	

	glm::quat s_asteroid_base_rotation;
	glm::quat m_asteroid_base_rotation;
	glm::quat l_asteroid_base_rotation;


	float wobble = 0.0f;

	//Shot controls
	float shotTimer = 0.0f;
	float shotDelay = .2f;
	
	//camera:
	Scene::Camera *camera = nullptr;

	float left_bound = -35.f;
	float right_bound = 35.f;
	float left_bound_movement = -25.f;
	float right_bound_movement = 25.f;
	float top_bound = 14.f;
	float bottom_bound = -14.f;

	float spawnDelay = 1.f;
	float minSpawnDelay = .1f;
	float spawnDelayChange = .01f;

	float spawnCounter = 0.0f;
	
	float time = 0.0f;

	bool game_running = true;

};
