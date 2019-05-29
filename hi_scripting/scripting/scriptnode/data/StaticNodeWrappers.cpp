/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;

scriptnode::HiseDspBase::ParameterData HardcodedNode::getParameter(const String& id)
{
	for (auto& c : internalParameterList)
		if (c.id == id)
			return c;

	return HiseDspBase::ParameterData("undefined");
}


scriptnode::HiseDspBase::ParameterData HardcodedNode::getParameter(const String& id, NormalisableRange<double> d)
{
	return getParameter(id).withRange(d);
}

bool HiseDspBase::isHardcoded() const
{
	return dynamic_cast<const HardcodedNode*>(this) != nullptr;
}


}

