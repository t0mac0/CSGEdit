#include <unordered_map>

#include "DifferenceCsgNode.hpp"

RT::DifferenceCsgNode::DifferenceCsgNode()
{}

RT::DifferenceCsgNode::~DifferenceCsgNode()
{}

std::list<RT::Intersection>	RT::DifferenceCsgNode::renderChildren(RT::Ray const & ray, unsigned int deph) const
{
  std::list<RT::Intersection>			uni = _children.front()->render(ray, deph);

  if (uni.empty())
    return std::list<RT::Intersection>();

  std::list<RT::Intersection>			dif;

  // Iterate through sub-tree to get intersections
  for (std::list<RT::AbstractCsgTree *>::const_iterator it = std::next(_children.begin()); it != _children.end(); it++)
    dif.merge((*it)->render(ray, deph));

  std::unordered_map<RT::AbstractCsgTree const *, bool>	inside;
  std::list<RT::Intersection>				result;
  unsigned int						state_uni = 0, state_dif = 0;

  std::list<RT::Intersection>::iterator		it_uni = uni.begin(), it_dif = dif.begin();

  // Iteration through positive object intersections
  while (it_uni != uni.end())
  {
    // If before next negative object
    if (it_dif == dif.end() || it_uni->distance < it_dif->distance)
    {
      // If outside negative object
      if (state_dif == 0)
	result.push_back(*it_uni);

      // Reverse inside/outside of positive object
      state_uni = state_uni == 1 ? 0 : 1;

      it_uni++;
    }
    // If after next negative object
    else
    {
      // If inside positive object and outside negative object
      if (state_uni != 0 && state_dif == 0)
	result.push_back(*it_dif);

      // Increment deepness if getting inside an object, decrement if getting outside
      state_dif += inside[it_dif->node] ? -1 : +1;
      inside[it_dif->node] = !(inside[it_dif->node]);

      // If inside positive object and outside negative object
      if (state_uni != 0 && state_dif == 0)
	result.push_back(*it_dif);

      it_dif++;
    }
  }

  // Reverse normal of negative objects
  for (RT::Intersection & it : result)
    if (it.node != _children.front())
      it.normal.d() *= -1.f;

  return result;
}