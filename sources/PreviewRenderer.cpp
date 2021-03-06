#include <iostream>
#include <SFML/System/Clock.hpp>

#include "PreviewRenderer.hpp"

RT::PreviewRenderer::PreviewRenderer()
  : _grid(), _scene(nullptr)
{}

RT::PreviewRenderer::~PreviewRenderer()
{
  stop();
}

void				RT::PreviewRenderer::load(RT::Scene * scene)
{
  stop();

  _scene = scene;

  // Reset zone grid
  _grid.resize((_scene->image().getSize().x / RT::Config::Renderer::BlockSize + (_scene->image().getSize().x % RT::Config::Renderer::BlockSize ? 1 : 0)) * (_scene->image().getSize().y / RT::Config::Renderer::BlockSize + (_scene->image().getSize().y % RT::Config::Renderer::BlockSize ? 1 : 0)));
  for (unsigned int i = 0; i < _grid.size(); i++)
    _grid[i] = RT::Config::Renderer::BlockSize;
}

void				RT::PreviewRenderer::begin()
{
  // If nothing to render, cancel
  if (_grid.size() == 0)
    return;

  std::list<std::thread>	threads;
  sf::Clock			clock;

  // Launch rendering threads
  for (unsigned int i = 0; i < _scene->config().threadNumber; i++)
    threads.push_back(std::thread((void(RT::PreviewRenderer::*)())(&RT::PreviewRenderer::preview), this));

  // Wait for rendering threads to finish
  for (std::thread & it : threads)
    it.join();

  if (active())
    std::cout << "[Preview] Completed in " << clock.getElapsedTime().asMilliseconds() / 1000 << "." << clock.getElapsedTime().asMicroseconds() % 1000 << "s." << std::endl;
}

void				RT::PreviewRenderer::preview()
{
  while (active())
  {
    unsigned int		r = (unsigned int)Math::Random::rand((double)_grid.size());
    unsigned int		z = (unsigned int)_grid.size();

    // Find a zone to render
    for (unsigned int a = RT::Config::Renderer::BlockSize; a > 0 && z == _grid.size(); a /= 2)
      for (unsigned int b = 0; b < _grid.size() && z == _grid.size(); b++)
	if (_grid[(r + b) % _grid.size()] == a)
	  z = (r + b) % _grid.size();

    // Return if no zone to render
    if (z == _grid.size())
      return;
    else
      preview(z);
  }
}

void				RT::PreviewRenderer::preview(unsigned int zone)
{
  unsigned int			size = _grid[zone];

  // Lock grid zone
  _grid[zone] = RT::Config::Renderer::BlockSize + 1;

  // Calcul zone coordinates (x, y)
  unsigned int			x = zone % (_scene->image().getSize().x / RT::Config::Renderer::BlockSize + (_scene->image().getSize().x % RT::Config::Renderer::BlockSize ? 1 : 0)) * RT::Config::Renderer::BlockSize;
  unsigned int			y = zone / (_scene->image().getSize().x / RT::Config::Renderer::BlockSize + (_scene->image().getSize().x % RT::Config::Renderer::BlockSize ? 1 : 0)) * RT::Config::Renderer::BlockSize;

  // Render zone
  for (unsigned int a = 0; a < RT::Config::Renderer::BlockSize && active(); a += size)
    for (unsigned int b = 0; b < RT::Config::Renderer::BlockSize && active(); b += size)
      if ((size == RT::Config::Renderer::BlockSize || a % (size * 2) != 0 || b % (size * 2) != 0) && x + a < _scene->image().getSize().x && y + b < _scene->image().getSize().y)
      {
	RT::Color clr = preview(x + a, y + b);

	for (unsigned int c = 0; c < size; c++)
	  for (unsigned int d = 0; d < size; d++)
	    if (x + a + c < _scene->image().getSize().x && y + b + d < _scene->image().getSize().y)
	      _scene->image().setPixel(x + a + c, y + b + d, clr.sfml());
      }

  if (active())
    _grid[zone] = size / 2;
  else
    _grid[zone] = size;
}

RT::Color			RT::PreviewRenderer::preview(unsigned int x, unsigned int y) const
{
  RT::Ray			ray;

  // Calcul ray according to (x, y) coordinates
  ray.d().x() = (double)_scene->image().getSize().x;
  ray.d().y() = (double)_scene->image().getSize().x / 2.f - x + 0.5f;
  ray.d().z() = (double)_scene->image().getSize().y / 2.f - y + 0.5f;

  // Virtual reality
  if (_scene->vr().offset != 0.f)
  {
    double  center;

    // Left eye
    if (ray.d().y() > 0.f)
    {
      center = +(double)_scene->image().getSize().x * _scene->vr().center / 4.f;
      ray.p().y() = +_scene->vr().offset / 2.f;
      ray.d().y() = ray.d().y() - _scene->image().getSize().x / 4.f;
    }
    // Right eye
    else
    {
      center = -(double)_scene->image().getSize().x * _scene->vr().center / 4.f;
      ray.p().y() = -_scene->vr().offset / 2.f;
      ray.d().y() = ray.d().y() + _scene->image().getSize().x / 4.f;
    }

    // Distortion
    if (_scene->vr().distortion != 0.f)
    {
      double  distortion = std::sqrt(std::pow(ray.d().y() - center, 2) + std::pow(ray.d().z(), 2)) / (std::sqrt(std::pow(_scene->image().getSize().x, 2) + std::pow(_scene->image().getSize().y, 2)) / std::abs(_scene->vr().distortion));

      if (_scene->vr().distortion > 0.f)
	distortion = distortion / std::atan(distortion);
      else
	distortion = std::atan(distortion) / distortion;

      ray.d().y() = (ray.d().y() - center) * distortion + center;
      ray.d().z() *= distortion;
    }
  }

  ray = (_scene->camera() * ray).normalize();

  // Render intersections using ray
  std::list<RT::Intersection>	intersect = _scene->csg()->render(ray);
  
  // Delete back intersections
  while (!intersect.empty() && intersect.front().distance < 0.f)
    intersect.pop_front();

  // Calcul intersection color if it exist, otherwise return black
  if (!intersect.empty())
    // Check for a correctly oriented normal
    if (RT::Ray::cos(ray.d(), intersect.front().normal.d()) < 0.f)
      return _scene->light()->preview(Math::Matrix<4, 4>::identite(), _scene, ray, intersect.front(), 0);
  // Return an error color if wrong normal
    else
      return RT::Color(1.f, (x / 2 + y / 5) / 2 % 2 ? 0.54f : 0.12f, (x / 2 + y / 5) / 2 % 2 ? 0.54f : 0.12f);
  else
    return RT::Color(0.f);
}

double				RT::PreviewRenderer::progress()
{
  for (unsigned int i : _grid)
    if (i != 0)
      return 0.f;
  return 1.f;
}
