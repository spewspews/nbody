#include "galaxy.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

static constexpr double ascale = 15, vscale = 5;

void Mouse::operator()(Galaxy& g) {
	update();
	ui_.sim_.unpause(0);
	switch(buttons_) {
	case SDL_BUTTON_LMASK:
		body(g);
		break;
	case SDL_BUTTON_MMASK:
		zoom(g);
		break;
	case SDL_BUTTON_RMASK:
		move(g);
		break;
	}
	ui_.sim_.pause(0);
}

inline void Mouse::update() {
	PauseGuard<Simulator> pg(ui_.sim_, 0);
	SDL_RenderPresent(ui_.renderer_);
	SDL_Delay(5);
	SDL_PumpEvents();
	buttons_ = SDL_GetMouseState(&p.x, &p.y);
	vp = ui_.toVector(p);
	SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
}

void Mouse::body(Galaxy& g) {
	PauseGuard<Simulator> pg(ui_.sim_, 1);
	Body b(ui_.scale_);
	b.p = vp;
	for(;;) {
		ui_.draw(g);
		ui_.draw(b);
		ui_.draw(b, b.v, vscale);
		update();
		if(!(buttons_ & SDL_BUTTON_LMASK))
			break;
		if(buttons_ == (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK))
			setSize(b, g);
		else if(buttons_ == (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
			setVel(b, g);
		else
			b.p = vp;
	}
	g.bodies.push_back(b);
	ui_.orig_ = ui_.toPoint(g.center());
	g.checkLimit(b.p);
	ui_.draw(g);
}

void Mouse::setSize(Body& b, const Galaxy& g) {
	auto oldp = p;
	for(;;) {
		auto d = vp - b.p;
		auto h = std::hypot(d.x, d.y);
		b.size = h == 0 ? 2.0 * ui_.scale_ : h;
		b.mass = b.size * b.size * b.size;
		ui_.draw(g);
		ui_.draw(b);
		ui_.draw(b, b.v, vscale);
		update();
		if(buttons_ != (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK))
			break;
	}
	SDL_WarpMouseInWindow(nullptr, oldp.x, oldp.y);
}

void Mouse::setVel(Body& b, const Galaxy& g) {
	auto oldp = p;
	for(;;) {
		b.v = (vp - b.p) / vscale;
		ui_.draw(g);
		ui_.draw(b);
		ui_.draw(b, b.v, vscale);
		update();
		if(buttons_ != (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
			break;
	}
	SDL_WarpMouseInWindow(nullptr, oldp.x, oldp.y);
}

inline void Mouse::zoom(const Galaxy& g) {
	auto op = p;
	auto oscale = ui_.scale_;
	Point sc;
	SDL_GetWindowSize(ui_.screen_, &sc.x, &sc.y);
	sc /= 2;
	for(;;) {
		update();
		if(buttons_ != SDL_BUTTON_MMASK)
			break;
		auto d = p - op;
		auto z = tanh((double)d.y / 200) + 1;
		auto gsc = ui_.toVector(sc);
		PauseGuard<Simulator> pg(ui_.sim_, 0);
		ui_.scale_ = z * oscale;
		auto off = ui_.toPoint(gsc) - sc;
		ui_.orig_ -= off;
		ui_.draw(g);
	}
}

inline void Mouse::move(const Galaxy& g) {
	auto oldp = p;
	for(;;) {
		update();
		if(buttons_ != SDL_BUTTON_RMASK)
			break;
		PauseGuard<Simulator> pg(ui_.sim_, 0);
		ui_.orig_ += p - oldp;
		oldp = p;
		ui_.draw(g);
	}
}

Vector UI::toVector(const Point& p) const {
	return {static_cast<double>(p.x - orig_.x) * scale_,
	        static_cast<double>(p.y - orig_.y) * scale_};
}

Point UI::toPoint(const Vector& v) const {
	return {static_cast<int>(v.x / scale_) + orig_.x,
	        static_cast<int>(v.y / scale_) + orig_.y};
}

void UI::draw(const Galaxy& g) const {
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
	SDL_RenderClear(renderer_);
	for(auto& b : g.bodies) {
		draw(b);
		if(showv_)
			draw(b, b.v, vscale);
		if(showa_)
			draw(b, b.a, ascale);
	}
	SDL_RenderPresent(renderer_);
}

void UI::draw(const Body& b) const {
	auto pos = toPoint(b.p);
	auto drawSize = static_cast<int>(b.size / scale_);

	auto err = filledCircleRGBA(renderer_, pos.x, pos.y, drawSize, b.r, b.g,
	                            b.b, 0xff);
	if(err == -1) {
		std::stringstream ss;
		ss << "Could not draw circle: " << SDL_GetError();
		throw std::runtime_error(ss.str());
	}
}

void UI::draw(const Body& b, const Vector& e, double scale) const {
	auto spos = toPoint(b.p);
	auto epos = toPoint(e * scale + b.p);
	auto err = aalineRGBA(renderer_, spos.x, spos.y, epos.x, epos.y, b.r, b.g,
	                      b.b, 0xff);
	if(err == -1) {
		std::stringstream ss;
		std::cerr << "Could not draw vector: " << SDL_GetError();
		throw std::runtime_error(ss.str());
	}
}

void UI::init() {
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::stringstream ss;
		ss << "Could not initialize SDL: " << SDL_GetError();
		throw std::runtime_error(ss.str());
	}
	atexit(SDL_Quit);

	screen_ = SDL_CreateWindow("Galaxy", SDL_WINDOWPOS_UNDEFINED,
	                           SDL_WINDOWPOS_UNDEFINED, 640, 640,
	                           SDL_WINDOW_RESIZABLE);
	if(!screen_) {
		std::stringstream ss;
		ss << "Could not create window: " << SDL_GetError();
		throw std::runtime_error(ss.str());
	}

	renderer_ = SDL_CreateRenderer(screen_, -1, 0);
	if(!renderer_) {
		std::stringstream ss;
		ss << "Could not create renderer: " << SDL_GetError() << '\n';
		throw std::runtime_error(ss.str());
	}

	SDL_GetWindowSize(screen_, &orig_.x, &orig_.y);
	orig_ /= 2;
}

void UI::center() {
	PauseGuard<Simulator> pg(sim_, 0);
	SDL_PumpEvents();
	SDL_FlushEvent(SDL_WINDOWEVENT_RESIZED);
	SDL_GetWindowSize(screen_, &orig_.x, &orig_.y);
	orig_ /= 2;
}

int UI::keyboard(SDL_Keycode& kc) {
	switch(kc) {
	case SDLK_a:
		showa_ = !showa_;
		break;
	case SDLK_v:
		showv_ = !showv_;
		break;
	case SDLK_q:
		return -1;
	case SDLK_SPACE:
		if(paused_) {
			sim_.unpause(1);
			paused_ = false;
		} else {
			sim_.pause(1);
			paused_ = true;
		}
		break;
	}
	return 0;
}

inline int UI::handleEvents(Galaxy& g) {
	PauseGuard<Simulator> pg(sim_, 0);
	SDL_Event e;
	while(SDL_PollEvent(&e)) {
		switch(e.type) {
		case SDL_QUIT:
			return -1;
		case SDL_KEYDOWN:
			if(keyboard(e.key.keysym.sym) == -1)
				return -1;
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouse_(g);
			break;
		}
	}
	return 0;
}

void UI::loop(Galaxy& g) {
	center();
	for(;;) {
		if(handleEvents(g) == -1) {
			sim_.exit();
			return;
		}
		SDL_Delay(100);
	}
}
