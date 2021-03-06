#ifndef _MATERIAL_HPP_
#define _MATERIAL_HPP_

#include <map>
#include <string>

#include "Color.hpp"

namespace RT
{
  namespace Config
  {
    namespace Material
    {
      unsigned int const	Quality(3);	// Default quality of material properties
    };
  };

  class Material
  {
  private:
    static std::map<std::string, RT::Material>	_material;	// Material mapped by name

    struct Illumination
    {
      double		emission = 1.f;					// Emitted/ambient light component multiplier
      double		diffuse = 1.f;					// Diffuse light component multiplier
      double		specular = 1.f;					// Specular light component multiplier
      double		shininess = 1.f;				// Shine coeffient of material [0.f-inf]
      unsigned int	quality = RT::Config::Material::Quality;	// Quality of light

      RT::Material::Illumination &	operator*=(RT::Material::Illumination const &);		// Illumination properties multiplication
      RT::Material::Illumination	operator*(RT::Material::Illumination const &) const;	// Illumination properties multiplication
    };

    struct Transparency
    {
      double		intensity = 0.f;				// Intensity of transparency [0.f-1.f]
      double		refraction = 1.f;				// Index of refraction [0.f-inf]
      double		glossiness = 1.f;				// Angle of glossiness [0.f-90.f]
      unsigned int	quality = RT::Config::Material::Quality;	// Quality of transparency

      RT::Material::Transparency &	operator*=(RT::Material::Transparency const &);		// Transparency properties multiplication
      RT::Material::Transparency	operator*(RT::Material::Transparency const &) const;	// Transparency properties multiplication
    };

    struct Reflection
    {
      double		intensity = 0.f;				// Intensity of reflection [0.f-1.f]
      double		glossiness = 1.f;				// Angle of glossiness [0.f-90.f]
      unsigned int	quality = RT::Config::Material::Quality;	// Quality of transparency

      RT::Material::Reflection &	operator*=(RT::Material::Reflection const &);		// Reflection properties multiplication
      RT::Material::Reflection		operator*(RT::Material::Reflection const &) const;	// Reflection properties multiplication
    };

  public:
    RT::Color			color;		// Color of material
    RT::Material::Illumination	direct;		// Direct illumination properties
    RT::Material::Illumination	indirect;	// Global illumination properties
    RT::Material::Transparency	transparency;	// Transparency properties
    RT::Material::Reflection	reflection;	// reflection properties

    Material();
    ~Material();

    RT::Material &		operator*=(RT::Material const &);	// Multiply materials properties
    RT::Material		operator*(RT::Material const &) const;	// Multiply materials properties

    static RT::Material	const &	getMaterial(std::string const &);			// Get material from his name
    static void			setMaterial(std::string const &, RT::Material const &);	// Set material properties
    static void			initialize();						// Initialize all materials
  };
};

#endif
