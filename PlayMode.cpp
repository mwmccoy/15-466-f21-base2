#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint asteroids_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > asteroids_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("asteroids.pnct"));
	asteroids_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
	});

Load< Scene > asteroids_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("asteroids.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		Mesh const& mesh = asteroids_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable& drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = asteroids_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		});
	});

PlayMode::PlayMode() : scene(*asteroids_scene) {

	Scene::Transform* s_asteroid_transform = nullptr;
	Scene::Transform* m_asteroid_transform = nullptr;
	Scene::Transform* l_asteroid_transform = nullptr;
	Scene::Transform* shot_transform = nullptr;
	
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "S_Asteroid") s_asteroid_transform = &transform;
		else if (transform.name == "M_Asteroid") m_asteroid_transform = &transform;
		else if (transform.name == "L_Asteroid") l_asteroid_transform = &transform;
		else if (transform.name == "Shot") shot_transform = &transform;
		else if (transform.name == "Ship") player = &transform;
	}

	for (auto& drawable : scene.drawables)
	{
		if (drawable.transform == s_asteroid_transform)
		{
			s_asteroid = &drawable;
		}
		else if (drawable.transform == m_asteroid_transform)
		{
			m_asteroid = &drawable;
		}
		else if (drawable.transform == l_asteroid_transform)
		{
			l_asteroid = &drawable;
		}
		else if (drawable.transform == shot_transform)
		{
			shot = &drawable;
		}
	}

	
	if (s_asteroid == nullptr) throw std::runtime_error("Small asteroid not found.");
	if (m_asteroid == nullptr) throw std::runtime_error("Medium asteroid not found.");
	if (l_asteroid == nullptr) throw std::runtime_error("Large asteroid not found.");
	if (shot == nullptr) throw std::runtime_error("Shot projectile not found.");
	if (player == nullptr) throw std::runtime_error("Player ship not found.");

	
	s_asteroid_base_rotation = s_asteroid_transform->rotation;
	m_asteroid_base_rotation = m_asteroid_transform->rotation;
	l_asteroid_base_rotation = l_asteroid_transform->rotation;

	//Hide our original meteors
	s_asteroid_transform->position.x += 100;
	m_asteroid_transform->position.x += 100;
	l_asteroid_transform->position.x += 100;
	shot_transform->position.x += 100;


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		/*
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}*/
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (!game_running)
	{
		simulate_physics(elapsed);
		return;
	}
	time += elapsed;

	//Update spawn behavior
	spawnDelay -= spawnDelayChange * elapsed;
	if (spawnDelay < minSpawnDelay)
	{
		spawnDelay = minSpawnDelay;
	}

	spawnCounter -= elapsed;
	if (spawnCounter < 0)
	{
		spawnCounter = spawnDelay;

		glm::vec3 spawnPos = glm::vec3(rand() % (int)(top_bound - bottom_bound) + bottom_bound, right_bound, 0.0f);
		float speed = (rand() % 8) * 5.f;

		switch (rand() % 3)
		{
			case 0:
				spawn_s_asteroid(spawnPos);
				velocities.push_back(speed);
				break;
			case 1:
				spawn_m_asteroid(spawnPos);
				velocities.push_back(speed);
				break;
			default:
				spawn_l_asteroid(spawnPos);
				velocities.push_back(speed);
				break;
		}
	}

	/*
	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	*/

	//move player
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		
		shotTimer -= elapsed;
		if (shotTimer < 0.f)
		{
			shotTimer = 0.f;
		}

		if (space.pressed && shotTimer <= 0.f)
		{

			spawn_shot(player->position + shotOffsetFromPlayer);
			shotTimer = shotDelay;
			
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//glm::mat4x3 frame = camera->transform->make_local_to_parent();
		//glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		//glm::vec3 forward = -frame[2];

		player->position += glm::vec3(-move.y, move.x, 0.f);

		if (player->position.x > top_bound) player->position.x = top_bound;
		if (player->position.x < bottom_bound) player->position.x = bottom_bound;

		if (player->position.y > right_bound_movement) player->position.y = right_bound_movement;
		if (player->position.y < left_bound_movement) player->position.y = left_bound_movement;

	}

	simulate_physics(elapsed);

	

	//Check asteroids for end of game condition
	{
		for (auto& asteroid : asteroids)
		{
			if (asteroid->position.y < left_bound)
			{
				end_game();
				break;
			}
		}
	}


	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::simulate_physics(float elapsed)
{
	//slowly rotates through [0,1):
	wobble += elapsed;
	//wobble -= std::floor(wobble);

	//Bad loop to use, but I need the index for variable speeds
	for (size_t i = 0; i < asteroids.size(); i++)
	{
		asteroids[i]->position = asteroids[i]->position + asteroidDirection * velocities[i] * elapsed;
		asteroids[i]->rotation = s_asteroid_base_rotation * glm::angleAxis(
			wobble,
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
	}

	for (auto& shotToMove : shots)
	{
		shotToMove->position = shotToMove->position + shotDirection * shotVelocity * elapsed;
	}

	//Clean up shots that make it offscreen
	{
		for (auto& shot_to_check : shots)
		{
			if (shot_to_check->position.y > right_bound)
			{
				delete_shot(shot_to_check);
			}
		}
	}

	//Collision checking for shots
	{
		for (auto& shot_to_check : shots)
		{
			bool should_delete = false;

			auto radii_iterator = radii.begin();
			for (auto& asteroid : asteroids)
			{
				if (glm::distance(asteroid->position, shot_to_check->position) < (*radii_iterator + shotRadius))
				{
					//Destroy asteroid and shot
					delete_asteroid(asteroid);
					should_delete = true;
					//radii_iterator--;
				}

				++radii_iterator;
			}

			if (should_delete)
			{
				delete_shot(shot_to_check);
			}
		}
	}
}


void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		std::string timeValue = std::to_string(time).substr(0, 4);

		if (!game_running)
		{
			timeValue = "Game over! Your time was " + timeValue + " seconds!";
		}

		constexpr float H = 0.09f;
		lines.draw_text(timeValue,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(timeValue,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

Scene::Transform *PlayMode::spawn_copy(Scene::Drawable *to_copy, Scene::Transform *parent)
{
	scene.transforms.emplace_back();
	Scene::Transform* copy_transform = &scene.transforms.back();

	//Copy over variables
	copy_transform->name = "Copy of " + to_copy->transform->name;
	copy_transform->rotation = to_copy->transform->rotation;
	copy_transform->position = to_copy->transform->position;
	copy_transform->scale = to_copy->transform->scale;
	copy_transform->parent = parent;

	scene.drawables.emplace_back(copy_transform);
	Scene::Drawable& drawable = scene.drawables.back();

	drawable.pipeline = to_copy->pipeline;

	return drawable.transform;
}

Scene::Transform* PlayMode::spawn_shot(glm::vec3 position)
{
	if (shot == nullptr) throw std::runtime_error("Scene should have shot object, but it does not.");
	Scene::Transform *new_shot = spawn_copy(shot, nullptr);
	new_shot->position = position;
	shots.push_back(new_shot);

	return new_shot;
}

Scene::Transform* PlayMode::spawn_s_asteroid(glm::vec3 position)
{
	if (s_asteroid == nullptr) throw std::runtime_error("Scene should have s_asteroid object, but it does not.");
	Scene::Transform* new_asteroid = spawn_copy(s_asteroid, nullptr);
	new_asteroid->position = position;
	new_asteroid->scale = glm::vec3(.5f, .5f, .5f);
	asteroids.push_back(new_asteroid);

	radii.push_back(.5f);

	return new_asteroid;
}

Scene::Transform* PlayMode::spawn_m_asteroid(glm::vec3 position)
{
	if (m_asteroid == nullptr) throw std::runtime_error("Scene should have m_asteroid object, but it does not.");
	Scene::Transform* new_asteroid = spawn_copy(m_asteroid, nullptr);
	new_asteroid->position = position;
	new_asteroid->scale = glm::vec3(1.f, 1.f, 1.f);
	asteroids.push_back(new_asteroid);

	radii.push_back(1.f);

	return new_asteroid;
}

Scene::Transform* PlayMode::spawn_l_asteroid(glm::vec3 position)
{
	if (l_asteroid == nullptr) throw std::runtime_error("Scene should have l_asteroid object, but it does not.");
	Scene::Transform* new_asteroid = spawn_copy(l_asteroid, nullptr);
	new_asteroid->position = position;
	new_asteroid->scale = glm::vec3(2.f, 2.f, 2.f);
	asteroids.push_back(new_asteroid);

	radii.push_back(2.f);

	return new_asteroid;
}


void PlayMode::delete_shot(Scene::Transform *shot_to_delete)
{
	if (shot_to_delete == nullptr) throw std::runtime_error("Can't delete a null shot!");

	//Collect some stats, to yell if something goes wrong.
	size_t shot_count = shots.size();
	size_t tran_count = scene.transforms.size();
	size_t draw_count = scene.drawables.size();

	//Remove it from our tracking list
	for (auto i = shots.begin(); i != shots.end(); i++)
	{
		if (*i == shot_to_delete)
		{
			i = shots.erase(i);
			--i;
		}
	}

	//Remove from scene transforms
	for (auto i = scene.transforms.begin(); i != scene.transforms.end(); i++)
	{
		if (&*i == shot_to_delete)
		{
			i = scene.transforms.erase(i);
			--i;
		}
	}

	//Remove from drawables
	for (auto i = scene.drawables.begin(); i != scene.drawables.end(); i++)
	{
		if (i->transform == shot_to_delete)
		{
			i = scene.drawables.erase(i);
			--i;
		}
	}

	if (shot_count == shots.size()) throw std::runtime_error("Item was not removed from shots list!");
	if (tran_count == scene.transforms.size()) throw std::runtime_error("Item was not removed from transform list!");
	if (draw_count == scene.drawables.size()) throw std::runtime_error("Item was not removed from Drawables list!");
}

void PlayMode::delete_asteroid(Scene::Transform* asteroid_to_delete)
{
	if (asteroid_to_delete == nullptr) throw std::runtime_error("Can't delete a null asteroid!");

	//Collect some stats, to yell if something goes wrong.
	size_t asteroids_count = asteroids.size();
	size_t tran_count = scene.transforms.size();
	size_t draw_count = scene.drawables.size();

	auto velocity_iterator = velocities.begin();
	auto radii_iterator = radii.begin();

	//Remove it from our tracking list
	for (auto i = asteroids.begin(); i != asteroids.end(); i++)
	{
		if (*i == asteroid_to_delete)
		{
			i = asteroids.erase(i);
			--i;

			//Remove the associated velocity
			velocities.erase(velocity_iterator);
			--velocity_iterator;

			//Remove the associated radius
			radii.erase(radii_iterator);
			--radii_iterator;
		}

		velocity_iterator++;
		radii_iterator++;
	}

	//Remove from scene transforms
	for (auto i = scene.transforms.begin(); i != scene.transforms.end(); i++)
	{
		if (&*i == asteroid_to_delete)
		{
			i = scene.transforms.erase(i);
			--i;
		}
	}

	//Remove from drawables
	for (auto i = scene.drawables.begin(); i != scene.drawables.end(); i++)
	{
		if (i->transform == asteroid_to_delete)
		{
			i = scene.drawables.erase(i);
			--i;
		}
	}

	if (asteroids_count != asteroids.size() + 1) throw std::runtime_error("Item was not removed from asteroids list!");
	if (tran_count == scene.transforms.size()) throw std::runtime_error("Item was not removed from transform list!");
	if (draw_count == scene.drawables.size()) throw std::runtime_error("Item was not removed from Drawables list!");
}

void PlayMode::end_game()
{
	std::cout << "Game is over!" << std::endl;
	game_running = false;
}