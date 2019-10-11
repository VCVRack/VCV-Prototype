/**
Factorial router
by Andrew Belt

There are 120 ways to route 5 inputs into 5 outputs.
This uses input 6 as a CV control to choose the particular permutation.
The CV is split into 120 intervals, each corresponding to a permutation of the inputs 1 through 5.
*/

function permutation(arr, k) {
	let n = arr.length
	// Clone array
	arr = arr.slice(0)
	// Compute n!
	let factorial = 1
	for (let i = 2; i <= n; i++) {
		factorial *= i
	}

	// Build permutation array by selecting the j'th element from the remaining elements until all are chosen.
	let perm = []
	for (let i = n; i >= 1; i--) {
		factorial /= i
		let j = Math.floor(k / factorial)
		k %= factorial
		let el = arr[j]
		arr.splice(j, 1)
		perm.push(el)
	}

	return perm
}

config.bufferSize = 16

function process(block) {
	// Get factorial index from input 6
	let k = Math.floor(block.inputs[5][0] / 10 * 120)
	k = Math.min(Math.max(k, 0), 120 - 1)
	// Get index set permutation
	let perm = permutation([1, 2, 3, 4, 5], k)
	display(perm.join(", "))
	for (let i = 0; i < 5; i++) {
		// Permute input i
		for (let j = 0; j < block.bufferSize; j++) {
			block.outputs[i][j] = block.inputs[perm[i] - 1][j]
		}
	}
}
