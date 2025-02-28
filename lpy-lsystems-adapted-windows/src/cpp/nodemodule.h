/* ---------------------------------------------------------------------------
 #
 #       L-Py: L-systems in Python
 #
 #       Copyright 2003-2008 UMR Cirad/Inria/Inra Dap - Virtual Plant Team
 #
 #       File author(s): F. Boudon (frederic.boudon@cirad.fr)
 #
 # ---------------------------------------------------------------------------
 #
 #                      GNU General Public Licence
 #
 #       This program is free software; you can redistribute it and/or
 #       modify it under the terms of the GNU General Public License as
 #       published by the Free Software Foundation; either version 2 of
 #       the License, or (at your option) any later version.
 #
 #       This program is distributed in the hope that it will be useful,
 #       but WITHOUT ANY WARRANTY; without even the implied warranty of
 #       MERCHANTABILITY or FITNESS For A PARTICULAR PURPOSE. See the
 #       GNU General Public License for more details.
 #
 #       You should have received a copy of the GNU General Public
 #       License along with this program; see the file COPYING. If not,
 #       write to the Free Software Foundation, Inc., 59
 #       Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 #
 # ---------------------------------------------------------------------------
 */

#pragma once

#include "axialtree.h"

LPY_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/

class LPY_API NodeModule : public ParamModule  {
public:

  NodeModule(AxialTree::const_iterator pos, 
			 AxialTree::const_iterator beg, 
			 AxialTree::const_iterator end);

  virtual ~NodeModule();

  inline NodeModule parent() const
  { return make_node(LPY::parent(m_pos,m_beg,m_end));}

  inline std::vector<NodeModule> children() const
  { return make_nodes(LPY::children(m_pos,m_end)); }

  inline std::vector<NodeModule> lateral_children() const
  { return make_nodes(LPY::lateral_children(m_pos,m_end)); }

  inline NodeModule direct_child() const
  { return make_node(LPY::direct_child(m_pos,m_end)); }

  inline NodeModule complex(int scale) const
  {  return make_node(LPY::complex(m_pos,scale,m_beg,m_end)); }

  inline NodeModule complex() const
  { return complex(scale()+1); }

  inline std::vector<NodeModule> components() const
  {  return make_nodes(LPY::components(m_pos,m_end)); }

  inline std::vector<NodeModule> components_at_scale(int scale) const
  {  return make_nodes(LPY::components_at_scale(m_pos, scale, m_end)); }

  inline bool isRoot() const
  { return m_pos == m_beg; }

  inline bool isValid() const
  { return m_pos != m_end; }

  inline size_t position() const
  { return std::distance(m_beg,m_pos); }

protected:

  inline std::vector<NodeModule> make_nodes(const std::vector<AxialTree::const_iterator>& pos) const
  { 
	  std::vector<NodeModule> res;
	  for(std::vector<AxialTree::const_iterator>::const_iterator itpos = pos.begin(); itpos != pos.end(); ++itpos)
		  res.push_back(NodeModule(*itpos,m_beg,m_end));
	  return res;
  }
  inline NodeModule make_node(const AxialTree::const_iterator& pos) const
  { 
	  if (pos == m_end) return NodeModule(m_beg,m_end);
	  return NodeModule(pos,m_beg,m_end);
  }

  NodeModule();

  NodeModule(AxialTree::const_iterator beg, 
			 AxialTree::const_iterator end);

  AxialTree::const_iterator m_pos;
  AxialTree::const_iterator m_beg; 
  AxialTree::const_iterator m_end;

};

/*---------------------------------------------------------------------------*/

LPY_END_NAMESPACE
