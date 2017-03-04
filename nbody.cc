#include "nbody.h"

static Galaxy glxy;
static UI ui;
static constexpr double dt = 0.2;
static double dt²;

std::ostream& operator<<(std::ostream& os, const Point& p) {
	os << "Point{" << p.x << ", " << p.y << "}";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Quad& q) {
	os << "Quad{p[" << q.p << "]}";
	return os;
}

Point& Point::operator=(const Point& p) {
	x = p.x;
	y = p.y;
	return *this;
}

Point& Point::operator/=(int s) {
	x /= s;
	y /= s;
	return *this;
}

Point& Point::operator+=(const Point& p) {
	x += p.x;
	y += p.y;
	return *this;
}

void simulate() {
	BHTree tree;
	for(;;) {
		glxy.mutex.lock();
		ui.draw(glxy);
		tree.calcforces(glxy);
		for(auto& b : glxy.bodies) {
			b.p += b.v * dt + b.a * dt² / 2;
			b.v += (b.a + b.newa) * dt / 2;
			glxy.checkLimit(b.p);
		}
		glxy.mutex.unlock();
		std::this_thread::sleep_for(std::chrono::nanoseconds{1});
	}
}

int main() {
	dt² = dt * dt;
	ui.init();
	ui.draw(glxy);
	std::thread simthread{simulate};
	ui.loop(glxy);
	std::cerr << "Error: ui loop exited\n";
	exit(1);
}
