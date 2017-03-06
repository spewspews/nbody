#include "galaxy.h"
#include <SDL2/SDL.h>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <random>
#include <thread>

class Point {
  public:
	int x, y;

	Point() : x{0}, y{0} {}
	Point(SDL_Point& p) : x{p.x}, y{p.y} {}
	Point(int a, int b) : x{a}, y{b} {}

	Point& operator=(const Point&);

	Point operator+(const Point& p) const { return {x + p.x, y + p.y}; }
	Point operator-(const Point& p) const { return {x - p.x, y - p.y}; }

	Point& operator+=(const Point&);
	Point& operator-=(const Point&);

	Point& operator/=(int);

	friend std::ostream& operator<<(std::ostream&, const Point&);
};

class Quad;

class QB {
  public:
	enum { empty, body, quad } t;
	union {
		Quad* q;
		const Body* b;
	};
	QB() : t{empty} {}
};

class Quad {
  public:
	Vector p;
	double mass;
	std::array<QB, 4> c;
	Quad() {}
	void setPosMass(const Body& b) {
		p = b.p;
		mass = b.mass;
	}
	friend std::ostream& operator<<(std::ostream&, const Quad&);
};

class BHTree {
	std::vector<Quad> quads_;
	QB root_;
	size_t i_, size_;
	double ε_, G_, θ_;

	bool insert(const Body&, double);
	void insert(Galaxy&);
	Quad* getQuad(const Body&);
	void calcforces(Body&, QB, double);
	void resize() {
		size_ *= 2;
		if(size_ > 10000) {
			std::cerr << "Too many quads\n";
			exit(1);
		}
		quads_.resize(size_);
	}

  public:
	BHTree() : quads_{5}, size_{quads_.size()}, ε_{500}, G_{1}, θ_{1} {};
	void calcforces(Galaxy&);
};

class UI;

class Simulator {
	std::atomic_bool pause_, paused_;
	std::condition_variable cvp_, cvpd_;
	std::mutex mup_, mupd_;
	std::thread t_;
	void simloop(Galaxy& g, UI& ui);

  public:
	double dt, dt²;
	Simulator() : pause_{false}, paused_{false}, dt{0.2}, dt²{dt * dt} {};

	void simulate(Galaxy& g, UI& ui);
	void pause();
	void unpause();
};

class Mouse {
	Uint32 buttons_;
	UI& ui_;

	void body(Galaxy&);
	void move(const Galaxy&);
	void setSize(Body&, const Galaxy&);
	void setVel(Body&, const Galaxy&);

  public:
	Mouse(UI& ui) : ui_{ui} {}
	Point p;
	Vector vp;

	void operator()(Galaxy&, Simulator&);
	void update();
};

class UI {
	friend class Mouse;
	Mouse mouse_;
	double scale_;
	SDL_Window* screen_;
	SDL_Renderer* renderer_;
	Point orig_;

	Vector toVector(const Point&) const;
	Point toPoint(const Vector&) const;
	void init();

  public:
	bool showv, showa;

	UI() : mouse_{*this}, scale_{30}, showv{false}, showa{false} { init(); }

	void draw(const Galaxy&) const;
	void draw(const Body&) const;
	void draw(const Body&, const Vector&) const;
	double defaultSize();
	void loop(Galaxy&, Simulator&);
};

extern bool paused;
