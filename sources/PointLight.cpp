#include "Exception.hpp"
#include "Math.hpp"
#include "PointLight.hpp"
#include "Scene.hpp"

RT::PointLight::PointLight(Math::Matrix<4, 4> const & transformation, RT::Color const & color, double radius, double intensity, double angle1, double angle2, unsigned int quality)
  : _color(color), _radius(radius), _intensity(intensity), _angle1(angle1), _angle2(angle2), _quality(quality)
{
  // Check values
  _radius = _radius > 0.f ? _radius : 0.f;
  _intensity = _intensity > 0.f ? _intensity : 0.f;
  _angle1 = _angle1 > 0.f ? _angle1 : 0.f;
  _angle1 = _angle1 < 180.f ? _angle1 : 180.f;
  _angle2 = _angle2 > _angle1 ? _angle2 : _angle1;
  _angle2 = _angle2 < 180.f ? _angle2 : 180.f;
  _quality = _quality > 1 ? _quality : 1;

  // Calculate position from tranformation matrix
  _position.d().x() = 1.f;
  _position = (transformation * _position).normalize();
}

RT::PointLight::~PointLight()
{}

RT::Color RT::PointLight::preview(RT::Scene const * scene, RT::Ray const & ray, RT::Ray const & normal, RT::Material const & material) const
{
  // If no ambient light, stop
  if (scene->config().lightDiffuse == 0.f || _color == RT::Color(0.f) || material.color == RT::Color(0.f) || material.diffuse == 0.f)
    return RT::Color(0.f);

  RT::Ray	light;
  
  // Inverse normal if necessary
  RT::Ray	n = normal;
  if (RT::Ray::cos(ray, normal) > 0)
    n.d() = normal.d() * -1.f;

  // Calculate intensity level
  double	intensity;
  if (_intensity == 0.f)
    intensity = 1.f;
  else
    intensity = (_intensity * _intensity) / ((_position.p().x() - n.p().x()) * (_position.p().x() - n.p().x()) + (_position.p().y() - n.p().y()) * (_position.p().y() - n.p().y()) + (_position.p().z() - n.p().z()) * (_position.p().z() - n.p().z()));
  
  // Set light ray from intersection to light origin
  light.d() = _position.p() - n.p();
  
  // Calculate normal cosinus with light ray
  double	diffuse = std::fmax(RT::Ray::cos(n, light), 0.f);
  if (diffuse == 0.f)
    return RT::Color(0.f);

  if (_angle1 == 0.f && _angle2 == 0.f)
    return material.color * material.diffuse * intensity * diffuse * _color * scene->config().lightDiffuse;
  else
  {
    double	angle = Math::Utils::RadToDeg(std::acos(-RT::Ray::cos(light, _position)));

    if (angle <= _angle1)
      return material.color * material.diffuse * intensity * diffuse * _color * scene->config().lightDiffuse;
    else if (_angle2 > _angle1 && angle < _angle2)
      return RT::Color((_angle2 - angle) / (_angle2 - _angle1)) * material.color * material.diffuse * intensity * diffuse * _color * scene->config().lightDiffuse;
    else
      return RT::Color(0.f);
  }
}

RT::Color RT::PointLight::render(RT::Scene const * scene, RT::Ray const & ray, RT::Ray const & normal, RT::Material const & material) const
{
  if ((scene->config().lightDiffuse == RT::Color(0.f) && scene->config().lightSpecular == RT::Color(0.f)) || (material.diffuse == 0.f && material.specular == 0.f) || material.transparency == 1.f || material.reflection == 1.f)
    return RT::Color(0.f);

  // Inverse normal if necessary
  RT::Ray	n = normal;
  if (RT::Ray::cos(ray, normal) > 0)
    n.d() = normal.d() * -1.f;

  std::list<RT::Ray>	rays;
  RT::Ray		r;
  RT::Color		diffuse, specular;

  r.p() = n.p() + n.d() * Math::Shift;
  
  if (_quality <= 1 || _radius == 0.f)
  {
    r.d() = _position.p() - r.p();
    rays.push_back(r);
  }
  else
  {
    // Random rotation matrix
    Math::Matrix<4, 4>	matrix = Math::Matrix<4, 4>::rotation(Math::Random::rand(180.f), Math::Random::rand(180.f), Math::Random::rand(180.f));

    for (double a = Math::Random::rand(_radius / (_quality + 1)); a < _radius; a += _radius / (_quality + 1))
      for (double b = Math::Random::rand(Math::Pi / (int)(a / _radius * _quality + 1)); b < Math::Pi; b += Math::Pi / (int)(a / _radius * _quality + 1))
	for (double c = Math::Random::rand(2.f * Math::Pi / (std::sin(b) * a / _radius * _quality + 1)); c < 2.f * Math::Pi; c += 2.f * Math::Pi / (std::sin(b) * a / _radius * _quality + 1))
	{
	  Math::Vector<4>	sphere;

	  sphere(0) = std::cos(b) * a;
	  sphere(1) = std::cos(c) * std::sin(b) * a;
	  sphere(2) = std::sin(c) * std::sin(b) * a;
	  
	  sphere = matrix * sphere;

	  r.d() = _position.p() + sphere - r.p();
	  rays.push_back(r);
	}
  }

  // Calculate reflection ray
  r.d() = n.p() - ray.p() - n.d() * 2.f * (n.d().x() * (n.p().x() - ray.p().x()) + n.d().y() * (n.p().y() - ray.p().y()) + n.d().z() * (n.p().z() - ray.p().z())) / (n.d().x() * n.d().x() + n.d().y() * n.d().y() + n.d().z() * n.d().z());
  
  // Render generated rays
  for (std::list<RT::Ray>::const_iterator it = rays.begin(); it != rays.end(); it++)
  {
    std::list<RT::Intersection>	intersect;
    RT::Color			light = RT::Color(_color);
    double			cos_d = RT::Ray::cos(n, *it);
    double			cos_s = RT::Ray::cos(r, *it);
    double			angle, intensity;

    if ((_angle1 == 0.f && _angle2 == 0.f) || (angle = acos(-RT::Ray::cos(_position, *it))) <= _angle1)
      angle = 1.f;
    else if (_angle2 > _angle1 && angle < _angle2)
      angle = (_angle2 - angle) / (_angle2 - _angle1);
    else
      angle = 0.f;
    
    if (_intensity == 0.f)
      intensity = 1.f;
    else
      intensity = (_intensity * _intensity) / (it->d().x() * it->d().x() + it->d().y() * it->d().y() + it->d().z() * it->d().z());

    if (angle != 0.f)
      intersect = scene->tree()->render((*it));

    // Render light
    while (!intersect.empty() && intersect.front().distance < 0.f)
      intersect.pop_front();
    while (!intersect.empty() && intersect.front().distance < 1.f && light != RT::Color(0.f))
    {
      light *= intersect.front().material.color * intersect.front().material.transparency;
      intersect.pop_front();
    }

    // Apply light to diffuse component
    diffuse += light * (cos_d > 0.f ? cos_d : -cos_d) * angle * intensity;

    // Apply light to specular component
    specular += light * pow((cos_s > 0.f ? cos_s : 0.f), material.shine) * angle * intensity;
  }

  return diffuse / (double)rays.size() * scene->config().lightDiffuse * material.color * material.diffuse * (1.f - material.transparency) * (1.f - material.reflection)
    + specular / (double)rays.size() * scene->config().lightSpecular * material.specular * (1.f - material.transparency) * (1.f - material.reflection);
}

std::string		RT::PointLight::dump() const
{
  std::stringstream	stream;
  Math::Matrix<4, 4>	transformation;
  double		ry, rz;

  // Calculate rotation angles of light
  ry = +std::asin(-_position.d().z());
  if (_position.d().x() != 0 || _position.d().y() != 0)
    rz = _position.d().y() > 0 ?
    +std::acos(-_position.d().x() / std::sqrt(_position.d().x() * _position.d().x() + _position.d().y() * _position.d().y())) :
    -std::acos(-_position.d().x() / std::sqrt(_position.d().x() * _position.d().x() + _position.d().y() * _position.d().y()));
  else
    rz = 0;

  stream << "transformation(" << (Math::Matrix<4, 4>::rotation(0, Math::Utils::RadToDeg(ry), Math::Utils::RadToDeg(rz)) * Math::Matrix<4, 4>::translation(_position.p().x(), _position.p().y(), _position.p().z())).dump() << ");point_light(" << _color.dump() << ", " << _radius << ", " << _intensity << ", " << _angle1 << ", " << _angle2 << ", " << _quality << ");end();";

  return stream.str();
}