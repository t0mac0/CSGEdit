#include "Exception.hpp"
#include "Math.hpp"
#include "PointLightLeaf.hpp"
#include "Scene.hpp"

RT::PointLightLeaf::PointLightLeaf(RT::Color const & color, double radius, double intensity, double angle1, double angle2)
  : _color(color), _radius(radius), _intensity(intensity), _angle1(angle1), _angle2(angle2 > angle1 ? angle2 : angle1)
{
  // Check values
  if (_radius < 0.f)
    throw RT::Exception(std::string(__FILE__) + ": l." + std::to_string(__LINE__));
  if (_intensity < 0.f)
    throw RT::Exception(std::string(__FILE__) + ": l." + std::to_string(__LINE__));
  if (_angle1 < 0.f || _angle1 > 180.f)
    throw RT::Exception(std::string(__FILE__) + ": l." + std::to_string(__LINE__));
  if (_angle2 < 0.f || _angle2 > 180.f)
    throw RT::Exception(std::string(__FILE__) + ": l." + std::to_string(__LINE__));
}

RT::PointLightLeaf::~PointLightLeaf()
{}

RT::Color	RT::PointLightLeaf::preview(Math::Matrix<4, 4> const & transformation, RT::Scene const *, RT::Ray const &, RT::Intersection const & intersection, unsigned int) const
{
  // If no diffuse light, stop
  if (intersection.material.color == 0.f || intersection.material.light.diffuse == 0.f)
    return RT::Color(0.f);
  
  // Calculate relative position of intersection
  RT::Ray	normal = transformation.inverse() * intersection.normal;

  // Calculate normal cosinus with light ray
  double	diffuse = RT::Ray::cos(normal.d(), Math::Vector<4>(-1.f, 0.f, 0.f, 0.f));
  if (diffuse < 0.f)
    return RT::Color(0.f);

  // Intensity of light
  double	intensity = _intensity == 0.f ? 1.f : ((_intensity * _intensity) / (normal.p().x() * normal.p().x() + normal.p().y() * normal.p().y() + normal.p().z() * normal.p().z()));

  if (_angle1 == 0.f && _angle2 == 0.f)
    return intersection.material.color * intersection.material.light.diffuse * intensity * diffuse * _color;
  else
  {
    double	angle = Math::Utils::RadToDeg(RT::Ray::angle(Math::Vector<4>(1.f, 0.f, 0.f, 0.f), normal.p()));

    if (angle < _angle1)
      return intersection.material.color * intersection.material.light.diffuse * intensity * diffuse * _color;
    else if (angle < _angle2)
      return RT::Color((_angle2 - angle) / (_angle2 - _angle1)) * intersection.material.color * intersection.material.light.diffuse * intensity * diffuse * _color;
    else
      return RT::Color(0.f);
  }
}

RT::Color RT::PointLightLeaf::render(Math::Matrix<4, 4> const & transformation, RT::Scene const * scene, RT::Ray const & ray, RT::Intersection const & intersection, unsigned int) const
{
  if (intersection.material.light.diffuse == 0.f && intersection.material.light.specular == 0.f)
    return RT::Color(0.f);

  // Inverse normal if necessary
  bool			inside = false;
  RT::Ray		normal = transformation.inverse() * intersection.normal;
  Math::Vector<4>	n = intersection.normal.d();
  if (RT::Ray::cos(ray.d(), intersection.normal.d()) > 0)
  {
    normal.d() *= -1.f;
    n *= -1.f;
    inside = true;
  }

  std::list<RT::Ray>	rays;

  Math::Vector<4>	p = normal.p() + normal.d() * Math::Shift;
  if (intersection.material.light.quality <= 1 || _radius == 0.f)
    rays.push_back(RT::Ray(Math::Vector<4>(0.f, 0.f, 0.f, 1.f), Math::Vector<4>(p.x(), p.y(), p.z(), 0.f)));
  else
  {
    // Random rotation matrix
    Math::Matrix<4, 4>	matrix = Math::Matrix<4, 4>::rotation(Math::Random::rand(180.f), Math::Random::rand(180.f), Math::Random::rand(180.f));

    for (double a = Math::Random::rand(_radius / (intersection.material.light.quality + 1)); a < _radius; a += _radius / (intersection.material.light.quality + 1))
      for (double b = Math::Random::rand(Math::Pi / (int)(a / _radius * intersection.material.light.quality + 1)); b < Math::Pi; b += Math::Pi / (int)(a / _radius * intersection.material.light.quality + 1))
	for (double c = Math::Random::rand(2.f * Math::Pi / (std::sin(b) * a / _radius * intersection.material.light.quality + 1)); c < 2.f * Math::Pi; c += 2.f * Math::Pi / (std::sin(b) * a / _radius * intersection.material.light.quality + 1))
	{
	  Math::Vector<4>	r = matrix * Math::Vector<4>(std::cos(b) * a, std::cos(c) * std::sin(b) * a, std::sin(c) * std::sin(b) * a, 1.f);

	  rays.push_back(RT::Ray(r, Math::Vector<4>(p.x() - r.x(), p.y() - r.y(), p.z() - r.z(), 0.f)));
	}
  }

  // Calculate reflection ray
  Math::Vector<4>	r = intersection.normal.p() - ray.p() - n * 2.f * (n.x() * (intersection.normal.p().x() - ray.p().x()) + n.y() * (intersection.normal.p().y() - ray.p().y()) + n.z() * (intersection.normal.p().z() - ray.p().z())) / (n.x() * n.x() + n.y() * n.y() + n.z() * n.z());
  
  // Render generated rays
  RT::Color		diffuse, specular;
  for (RT::Ray const & it : rays)
  {
    std::list<RT::Intersection>	intersect;
    RT::Color			light = RT::Color(_color);
    double			angle, intensity;

    if ((_angle1 == 0.f && _angle2 == 0.f) || (angle = Math::Utils::RadToDeg(RT::Ray::angle(Math::Vector<4>(1.f, 0.f, 0.f, 0.f), it.d()))) <= _angle1)
      angle = 1.f;
    else if (angle < _angle2)
      angle = (_angle2 - angle) / (_angle2 - _angle1);
    else
      angle = 0.f;
    
    if (_intensity == 0.f)
      intensity = 1.f;
    else
      intensity = (_intensity * _intensity) / (it.d().x() * it.d().x() + it.d().y() * it.d().y() + it.d().z() * it.d().z());

    if (angle != 0.f)
      intersect = scene->csg()->render(transformation * it);

    // Render light
    while (!intersect.empty() && intersect.front().distance < 0.f)
      intersect.pop_front();
    while (!intersect.empty() && intersect.front().distance < 1.f && light != 0.f)
    {
      light *= intersect.front().material.color * intersect.front().material.transparency.intensity;
      intersect.pop_front();
    }

    // Apply light to diffuse component
    diffuse += light * std::fabs(RT::Ray::cos(normal.d(), Math::Vector<4>(-it.d().x(), -it.d().y(), -it.d().z(), 0.f))) * angle * intensity;

    // Apply light to specular component
    if (inside == false)
      specular += light * pow(fmax(RT::Ray::cos(r, transformation * it.d() * -1.f), 0.f), intersection.material.light.shininess) * angle * intensity;
  }

  return diffuse / (double)rays.size() * intersection.material.color * intersection.material.light.diffuse * (1.f - intersection.material.transparency.intensity) * (1.f - intersection.material.reflection.intensity)
    + specular / (double)rays.size() * intersection.material.light.specular;
}