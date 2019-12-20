// Represents a process block in VCV Prototype. Users should generally not create instances
// of this class themselves, and should only modify the values in `outputs`, `knobs`, `lights`,
// and `switchLights`.
//
// Original author: Brian Heim

VcvPrototypeProcessBlock {
	classvar <>numRows; // Set internally, do not modify

	// Code in SuperColliderEngine.cpp relies on ordering.
	var <sampleRate; // Float
	var <sampleTime; // Float
	var <bufferSize; // Integer
	var <inputs; // Array[numRows] of Signal[bufferSize]
	var <>outputs; // Array[numRows] of Signal[bufferSize]
	var <>knobs; // FloatArray[numRows]
	var <switches; // Array[numRows] of Boolean
	var <>lights; // Array[numRows] of FloatArray[3]
	var <>switchLights; // Array[numRows] of FloatArray[3]

	// TODO not needed?
	*new { |... args| ^super.newCopyArgs(*args); }
}
