/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace snex {
using namespace juce;




CallbackCollection::CallbackCollection()
{
	bestCallback[FrameProcessing] = CallbackTypes::Inactive;
	bestCallback[BlockProcessing] = CallbackTypes::Inactive;
}

juce::String CallbackCollection::getBestCallbackName(int processType) const
{
	auto cb = bestCallback[processType];

	if (cb == CallbackTypes::Channel) return "Channel";
	if (cb == CallbackTypes::Frame) return "Frame";
	if (cb == CallbackTypes::Sample) return "Sample";

	return "Inactive";
}

void CallbackCollection::setupCallbacks()
{
	StringArray cIds = { "processChannel", "processFrame", "processSample" };

	using namespace snex::Types;

	prepareFunction = obj["prepare"];

	if (!prepareFunction.matchesArgumentTypes(ID::Void, { ID::Double, ID::Integer, ID::Integer }))
		prepareFunction = {};

	resetFunction = obj["reset"];

	if (!resetFunction.matchesArgumentTypes(ID::Void, {}))
		resetFunction = {};

	eventFunction = obj["handleEvent"];

	if (!eventFunction.matchesArgumentTypes(ID::Void, { ID::Event }))
		eventFunction = {};

	callbacks[CallbackTypes::Sample] = obj[cIds[CallbackTypes::Sample]];

	if (!callbacks[CallbackTypes::Sample].matchesArgumentTypes(ID::Float, { ID::Float }))
		callbacks[CallbackTypes::Sample] = {};

	callbacks[CallbackTypes::Frame] = obj[cIds[CallbackTypes::Frame]];

	if (!callbacks[CallbackTypes::Frame].matchesArgumentTypes(ID::Void, { ID::Block }))
		callbacks[CallbackTypes::Frame] = {};

	callbacks[CallbackTypes::Channel] = obj[cIds[CallbackTypes::Channel]];

	if (!callbacks[CallbackTypes::Channel].matchesArgumentTypes(ID::Void, { ID::Block, ID::Integer }))
		callbacks[CallbackTypes::Channel] = {};

	bestCallback[FrameProcessing] = getBestCallback(FrameProcessing);
	bestCallback[BlockProcessing] = getBestCallback(BlockProcessing);

	parameters.clear();

	auto parameterNames = ParameterHelpers::getParameterNames(obj);

	for (auto n : parameterNames)
	{
		auto pFunction = ParameterHelpers::getFunction(n, obj);

		if (!pFunction.matchesArgumentTypes(ID::Void, { ID::Double }))
			pFunction = {};

		parameters.add({ n, pFunction });
	}

	if (listener != nullptr)
		listener->initialised(*this);
}

int CallbackCollection::getBestCallback(int processType) const
{
	if (processType == FrameProcessing)
	{
		if (callbacks[CallbackTypes::Frame])
			return CallbackTypes::Frame;
		if (callbacks[CallbackTypes::Sample])
			return CallbackTypes::Sample;
		if (callbacks[CallbackTypes::Channel])
			return CallbackTypes::Channel;

		return CallbackTypes::Inactive;
	}
	else
	{
		if (callbacks[CallbackTypes::Channel])
			return CallbackTypes::Channel;
		if (callbacks[CallbackTypes::Frame])
			return CallbackTypes::Frame;
		if (callbacks[CallbackTypes::Sample])
			return CallbackTypes::Sample;

		return CallbackTypes::Inactive;
	}
}

void CallbackCollection::prepare(double sampleRate, int blockSize, int numChannels)
{
	if (sampleRate == -1 || blockSize == 0 || numChannels == 0)
		return;

	if (prepareFunction)
		prepareFunction.callVoid(sampleRate, blockSize, numChannels);

	if (resetFunction)
		resetFunction.callVoid();

	if (listener != nullptr)
		listener->prepare(sampleRate, blockSize, numChannels);
}

void CallbackCollection::setListener(Listener* l)
{
	listener = l;
}

snex::jit::FunctionData ParameterHelpers::getFunction(const String& parameterName, JitObject& obj)
{
	Identifier id("set" + parameterName);

	auto f = obj[id];

	if (f.matchesArgumentTypes(Types::ID::Void, { Types::ID::Double }))
		return f;

	return {};
}

juce::StringArray ParameterHelpers::getParameterNames(JitObject& obj)
{
	auto ids = obj.getFunctionIds();
	StringArray sa;

	for (int i = 0; i < ids.size(); i++)
	{
		auto fName = ids[i].toString();

		if (fName.startsWith("set"))
			sa.add(fName.fromFirstOccurrenceOf("set", false, false));
	}

	return sa;
}

JitExpression::JitExpression(const String& s, DebugHandler* handler) :
	memory(0)
{
	String code = "double get(double input){ return " + s + ";}";

	

	snex::jit::Compiler c(memory);
	obj = c.compileJitObject(code);

	if (c.getCompileResult().wasOk())
	{
		f = obj["get"];

		// Add this after the compilation, we don't want to spam the logger
		// with compilation messages
		if (handler != nullptr)
			memory.addDebugHandler(handler);
	}
	else
		errorMessage = c.getCompileResult().getErrorMessage();
}

double JitExpression::getValue(double input) const
{
	if (f)
		return f.callUncheckedWithCopy<double>(input);
	else
		return input;
}

juce::String JitExpression::getErrorMessage() const
{
	return errorMessage;
}

bool JitExpression::isValid() const
{
	return (bool)f;
}

juce::String JitExpression::convertToValidCpp(String input)
{
	return input.replace("Math.", "hmath::");
}

}