#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/windowModule/components.h"
#include "ecsModule/cameraModule/module.h"
#include "core/app_state.h"

#include "SDL3/SDL.h"
#include "spdlog/spdlog.h"

using namespace se;

int se_key_to_sdl_key(Key key) {
	switch(key) {
		case Key::F1:         return SDL_SCANCODE_F1;
		case Key::F2:         return SDL_SCANCODE_F2;
		case Key::F3:         return SDL_SCANCODE_F3;
		case Key::F4:         return SDL_SCANCODE_F4;
		case Key::F5:         return SDL_SCANCODE_F5;
		case Key::F6:         return SDL_SCANCODE_F6;
		case Key::F7:         return SDL_SCANCODE_F7;
		case Key::F8:         return SDL_SCANCODE_F8;
		case Key::F9:         return SDL_SCANCODE_F9;
		case Key::F10:        return SDL_SCANCODE_F10;
		case Key::F11:        return SDL_SCANCODE_F11;
		case Key::F12:        return SDL_SCANCODE_F12;
		case Key::Space:      return SDL_SCANCODE_SPACE;
		case Key::Apostrophe: return SDL_SCANCODE_APOSTROPHE;
		case Key::Comma:      return SDL_SCANCODE_COMMA;
		//case Key::Hyphen:   return SDL_SCANCODE_HYPHEN;
		case Key::Period:     return SDL_SCANCODE_PERIOD;
		case Key::Slash:      return SDL_SCANCODE_SLASH;
		case Key::Num0:       return SDL_SCANCODE_KP_0;
		case Key::Num1:       return SDL_SCANCODE_KP_1;
		case Key::Num2:       return SDL_SCANCODE_KP_2;
		case Key::Num3:       return SDL_SCANCODE_KP_3;
		case Key::Num4:       return SDL_SCANCODE_KP_4;
		case Key::Num5:       return SDL_SCANCODE_KP_5;
		case Key::Num6:       return SDL_SCANCODE_KP_6;
		case Key::Num7:       return SDL_SCANCODE_KP_7;
		case Key::Num8:       return SDL_SCANCODE_KP_8;
		case Key::Num9:       return SDL_SCANCODE_KP_9;
		case Key::Semicolon:  return SDL_SCANCODE_SEMICOLON;
		case Key::Equal:      return SDL_SCANCODE_EQUALS;
		case Key::A:          return SDL_SCANCODE_A;
		case Key::B:          return SDL_SCANCODE_B;
		case Key::C:          return SDL_SCANCODE_C;
		case Key::D:          return SDL_SCANCODE_D;
		case Key::E:          return SDL_SCANCODE_E;
		case Key::F:          return SDL_SCANCODE_F;
		case Key::G:          return SDL_SCANCODE_G;
		case Key::H:          return SDL_SCANCODE_H;
		case Key::I:          return SDL_SCANCODE_I;
		case Key::J:          return SDL_SCANCODE_J;
		case Key::K:          return SDL_SCANCODE_K;
		case Key::L:          return SDL_SCANCODE_L;
		case Key::M:          return SDL_SCANCODE_M;
		case Key::N:          return SDL_SCANCODE_N;
		case Key::O:          return SDL_SCANCODE_O;
		case Key::P:          return SDL_SCANCODE_P;
		case Key::Q:          return SDL_SCANCODE_Q;
		case Key::R:          return SDL_SCANCODE_R;
		case Key::S:          return SDL_SCANCODE_S;
		case Key::T:          return SDL_SCANCODE_T;
		case Key::U:          return SDL_SCANCODE_U;
		case Key::V:          return SDL_SCANCODE_V;
		case Key::W:          return SDL_SCANCODE_W;
		case Key::X:          return SDL_SCANCODE_X;
		case Key::Y:          return SDL_SCANCODE_Y;
		case Key::Z:          return SDL_SCANCODE_Z;
		case Key::LBracket:   return SDL_SCANCODE_LEFTBRACKET;
		case Key::Backslash:  return SDL_SCANCODE_BACKSLASH;
		case Key::RBracket:   return SDL_SCANCODE_RIGHTBRACKET;
		case Key::Grave:      return SDL_SCANCODE_GRAVE;
		case Key::Escape:     return SDL_SCANCODE_ESCAPE;
		case Key::Enter:      return SDL_SCANCODE_KP_ENTER;
		case Key::Tab:        return SDL_SCANCODE_TAB;
		case Key::Backspace:  return SDL_SCANCODE_BACKSPACE;
		case Key::Insert:     return SDL_SCANCODE_INSERT;
		case Key::Delete:     return SDL_SCANCODE_DELETE;
		case Key::Right:      return SDL_SCANCODE_RIGHT;
		case Key::Left:       return SDL_SCANCODE_LEFT;
		case Key::Down:       return SDL_SCANCODE_DOWN;
		case Key::Up:         return SDL_SCANCODE_UP;
		case Key::PageUp:     return SDL_SCANCODE_PAGEUP;
		case Key::PageDown:   return SDL_SCANCODE_PAGEDOWN;
		case Key::Home:       return SDL_SCANCODE_HOME;
		case Key::End:        return SDL_SCANCODE_END;
		case Key::LShift:     return SDL_SCANCODE_LSHIFT;
		case Key::LControl:   return SDL_SCANCODE_LCTRL;
		case Key::LAlt:       return SDL_SCANCODE_LALT;
		case Key::RShift:     return SDL_SCANCODE_RSHIFT;
		case Key::RControl:   return SDL_SCANCODE_RCTRL;
		case Key::RAlt:       return SDL_SCANCODE_RALT;
		default:              return SDL_SCANCODE_COUNT;
	}
}

Key sdl_key_to_se_key(int key) {
	switch(key) {
		case SDL_SCANCODE_F1:             return Key::F1;
		case SDL_SCANCODE_F2:             return Key::F2;
		case SDL_SCANCODE_F3:             return Key::F3;
		case SDL_SCANCODE_F4:             return Key::F4;
		case SDL_SCANCODE_F5:             return Key::F5;
		case SDL_SCANCODE_F6:             return Key::F6;
		case SDL_SCANCODE_F7:             return Key::F7;
		case SDL_SCANCODE_F8:             return Key::F8;
		case SDL_SCANCODE_F9:             return Key::F9;
		case SDL_SCANCODE_F10:            return Key::F10;
		case SDL_SCANCODE_F11:            return Key::F11;
		case SDL_SCANCODE_F12:            return Key::F12;
		case SDL_SCANCODE_SPACE:          return Key::Space;
		case SDL_SCANCODE_APOSTROPHE:     return Key::Apostrophe;
		case SDL_SCANCODE_COMMA:          return Key::Comma;
		//case SDL_SCANCODE_HYPHEN:       return Key::Hyphen;
		case SDL_SCANCODE_PERIOD:         return Key::Period;
		case SDL_SCANCODE_SLASH:          return Key::Slash;
		case SDL_SCANCODE_KP_0:           return Key::Num0;
		case SDL_SCANCODE_KP_1:           return Key::Num1;
		case SDL_SCANCODE_KP_2:           return Key::Num2;
		case SDL_SCANCODE_KP_3:           return Key::Num3;
		case SDL_SCANCODE_KP_4:           return Key::Num4;
		case SDL_SCANCODE_KP_5:           return Key::Num5;
		case SDL_SCANCODE_KP_6:           return Key::Num6;
		case SDL_SCANCODE_KP_7:           return Key::Num7;
		case SDL_SCANCODE_KP_8:           return Key::Num8;
		case SDL_SCANCODE_KP_9:           return Key::Num9;
		case SDL_SCANCODE_SEMICOLON:      return Key::Semicolon;
		case SDL_SCANCODE_EQUALS:         return Key::Equal;
		case SDL_SCANCODE_A:              return Key::A;
		case SDL_SCANCODE_B:              return Key::B;
		case SDL_SCANCODE_C:              return Key::C;
		case SDL_SCANCODE_D:              return Key::D;
		case SDL_SCANCODE_E:              return Key::E;
		case SDL_SCANCODE_F:              return Key::F;
		case SDL_SCANCODE_G:              return Key::G;
		case SDL_SCANCODE_H:              return Key::H;
		case SDL_SCANCODE_I:              return Key::I;
		case SDL_SCANCODE_J:              return Key::J;
		case SDL_SCANCODE_K:              return Key::K;
		case SDL_SCANCODE_L:              return Key::L;
		case SDL_SCANCODE_M:              return Key::M;
		case SDL_SCANCODE_N:              return Key::N;
		case SDL_SCANCODE_O:              return Key::O;
		case SDL_SCANCODE_P:              return Key::P;
		case SDL_SCANCODE_Q:              return Key::Q;
		case SDL_SCANCODE_R:              return Key::R;
		case SDL_SCANCODE_S:              return Key::S;
		case SDL_SCANCODE_T:              return Key::T;
		case SDL_SCANCODE_U:              return Key::U;
		case SDL_SCANCODE_V:              return Key::V;
		case SDL_SCANCODE_W:              return Key::W;
		case SDL_SCANCODE_X:              return Key::X;
		case SDL_SCANCODE_Y:              return Key::Y;
		case SDL_SCANCODE_Z:              return Key::Z;
		case SDL_SCANCODE_LEFTBRACKET:    return Key::LBracket;
		case SDL_SCANCODE_BACKSLASH:      return Key::Backslash;
		case SDL_SCANCODE_RIGHTBRACKET:   return Key::RBracket;
		case SDL_SCANCODE_GRAVE:          return Key::Grave;
		case SDL_SCANCODE_ESCAPE:         return Key::Escape;
		case SDL_SCANCODE_KP_ENTER:       return Key::Enter;
		case SDL_SCANCODE_TAB:            return Key::Tab;
		case SDL_SCANCODE_BACKSPACE:      return Key::Backspace;
		case SDL_SCANCODE_INSERT:         return Key::Insert;
		case SDL_SCANCODE_DELETE:         return Key::Delete;
		case SDL_SCANCODE_RIGHT:          return Key::Right;
		case SDL_SCANCODE_LEFT:           return Key::Left;
		case SDL_SCANCODE_DOWN:           return Key::Down;
		case SDL_SCANCODE_UP:             return Key::Up;
		case SDL_SCANCODE_PAGEUP:         return Key::PageUp;
		case SDL_SCANCODE_PAGEDOWN:       return Key::PageDown;
		case SDL_SCANCODE_HOME:           return Key::Home;
		case SDL_SCANCODE_END:            return Key::End;
		case SDL_SCANCODE_LSHIFT:         return Key::LShift;
		case SDL_SCANCODE_LCTRL:          return Key::LControl;
		case SDL_SCANCODE_LALT:           return Key::LAlt;
		case SDL_SCANCODE_RSHIFT:         return Key::RShift;
		case SDL_SCANCODE_RCTRL:          return Key::RControl;
		case SDL_SCANCODE_RALT:           return Key::RAlt;
		default:                          return Key::Unknown;
	}
}

InputModule::InputModule(flecs::world& world) {
	world.module<InputModule>();

	world.import<WindowModule>();
	world.import<CameraModule>();

	world.component<Input>().add(flecs::Singleton);

	world.system<Input, AppState>()
		.kind(Phases::HandleInput)
		.each([&world](Input& input, AppState& app_state) {
			for (auto& [key, state] : input.keys) {
				state.pressed = false;
				state.released = false;
			}

			input.mouse.wheel = glm::vec2(0.f);
			input.mouse.left.pressed = false;
			input.mouse.left.released = false;
			input.mouse.right.pressed = false;
			input.mouse.right.released = false;
			input.mouse.moved = false;

			SDL_Event event;

			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
					world.each([&](Window& window) {
						if (event.window.windowID != SDL_GetWindowID(window.handle)) {
							return;
						}

						window.has_focus = event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
					});
				}

				if (event.type == SDL_EVENT_QUIT) {
					app_state = AppState::Exit;
				}
				else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
					world.each([&](flecs::entity window_entity, Window& window) {
						if (event.window.windowID != SDL_GetWindowID(window.handle)) {
							return;
						}

						WindowResize event{ 
							.window_entity = window_entity
						};

						SDL_GetWindowSize(window.handle, &event.width, &event.height);

						world.each<Camera>([&](flecs::entity entity, Camera& camera) {
							if (std::holds_alternative<flecs::entity_t>(camera.render_target) && std::get<flecs::entity_t>(camera.render_target) != window_entity) {
								return;
							}

							world.event<WindowResize>()
								.id<Camera>()
								.entity(entity)
								.ctx(event)
								.emit();
						});

						window.width = event.width;
						window.height = event.height;
					});
				}
				else if (event.type == SDL_EVENT_WINDOW_OCCLUDED || event.type == SDL_EVENT_WINDOW_HIDDEN) {
					world.each([&](flecs::entity window_entity, Window& window) {
						if (event.window.windowID != SDL_GetWindowID(window.handle)) {
							return;
						}

						window.window_state = Window::WindowState::Hidden;
					});
				}
				else if (event.type == SDL_EVENT_WINDOW_EXPOSED) {
					world.each([&](flecs::entity window_entity, Window& window) {
						if (event.window.windowID != SDL_GetWindowID(window.handle)) {
							return;
						}

						window.window_state = Window::WindowState::Present;
					});
				}
				else if (event.type == SDL_EVENT_KEY_DOWN) {
					input.keys[sdl_key_to_se_key(event.key.scancode)].pressed = !input.keys[sdl_key_to_se_key(event.key.scancode)].remain;
					input.keys[sdl_key_to_se_key(event.key.scancode)].remain = true;
					input.keys[sdl_key_to_se_key(event.key.scancode)].released = false;
				}
				else if (event.type == SDL_EVENT_KEY_UP) {
					input.keys[sdl_key_to_se_key(event.key.scancode)].pressed = false;
					input.keys[sdl_key_to_se_key(event.key.scancode)].remain = false;
					input.keys[sdl_key_to_se_key(event.key.scancode)].released = true;
				}
				else if (event.type == SDL_EVENT_MOUSE_MOTION) {
					input.mouse.position.x = event.motion.x;
					input.mouse.position.y = event.motion.y;
				}
				else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						input.mouse.left.pressed = !input.mouse.left.remain;
						input.mouse.left.remain = true;
						input.mouse.left.released = false;
					}
					else if (event.button.button == SDL_BUTTON_RIGHT) {
						input.mouse.right.pressed = !input.mouse.left.remain;
						input.mouse.right.remain = true;
						input.mouse.right.released = false;
					}
				}
				else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
					if (event.button.button == SDL_BUTTON_LEFT) {
						input.mouse.left.pressed = false;
						input.mouse.left.remain = false;
						input.mouse.left.released = true;
					}
					else if (event.button.button == SDL_BUTTON_RIGHT) {
						input.mouse.right.pressed = false;
						input.mouse.right.remain = false;
						input.mouse.right.released = true;
					}
				}
				else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
					input.mouse.wheel.x = event.wheel.x;
					input.mouse.wheel.y = event.wheel.y;
				}
			}
		});

	world.add<Input>();
}
