#include <hf-risc.h>
#include <math.h>
#include <fixed.h>

#define N_PATTERNS			16
#define INPUT_NEURONS		7
#define HIDDEN_NEURONS		8
#define OUTPUT_NEURONS		4
#define ETA					0.30
#define ALPHA				0.90
#define EPSILON				0.0125
#define INITIAL_WEIGHT_MAX	0.50
#define EPOCHS_MAX			1000
#define STATUS_CYCLES		1000

struct neural_net_weights_s {
	fixed_t hidden_weights[(INPUT_NEURONS+1) * HIDDEN_NEURONS];
	fixed_t output_weights[(HIDDEN_NEURONS+1) * OUTPUT_NEURONS];
};

struct neural_net_s {
	char input[N_PATTERNS * INPUT_NEURONS];
	char target[N_PATTERNS * OUTPUT_NEURONS];
	fixed_t hidden[HIDDEN_NEURONS];
	fixed_t output[OUTPUT_NEURONS];
	struct neural_net_weights_s weights;
};

fixed_t sigmoid(fixed_t val)
{
	return fix_div(fix_val(1.0), (fix_val(1.0) + fix_exp(-val)));
}

void load(struct neural_net_s *net, char *input, char *target)
{
	int i, j;

	for (i = 0; i < N_PATTERNS; i++) {
		for (j = 0; j < INPUT_NEURONS; j++)
			net->input[i * INPUT_NEURONS + j] = input[i * INPUT_NEURONS + j];
		for (j = 0; j < OUTPUT_NEURONS; j++)
			net->target[i * OUTPUT_NEURONS + j] = target[i * OUTPUT_NEURONS + j];
	}
}

void infer(struct neural_net_s *net, int p)
{
	int i, j;
	fixed_t acc;

	// compute hidden layer activations
	for (i = 0; i < HIDDEN_NEURONS; i++) {
		acc = net->weights.hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i];
		for (j = 0; j < INPUT_NEURONS; j++)
			acc += fix_mul(fix_val((float) net->input[p * INPUT_NEURONS + j]), net->weights.hidden_weights[j * HIDDEN_NEURONS + i]);
		net->hidden[i] = sigmoid(acc);
	}

	// compute output layer activations and calculate errors
	for (i = 0; i < OUTPUT_NEURONS; i++) {
		acc = net->weights.output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i];
		for (j = 0; j < HIDDEN_NEURONS; j++)
			acc += fix_mul(net->hidden[j], net->weights.output_weights[j * OUTPUT_NEURONS + i]);
		net->output[i] = sigmoid(acc);
	}
}

void show_training(struct neural_net_s *net)
{
	int i, p;
	char buf[30];

	for (p = 0; p < N_PATTERNS; p++) {
		printf("\ninput (%02d): ", p);
		for (i = 0; i < INPUT_NEURONS; i++)
			printf("%d", net->input[p * INPUT_NEURONS + i]);
		printf(" target: ");
		for (i = 0; i < OUTPUT_NEURONS; i++)
			printf("%d", net->target[p * OUTPUT_NEURONS + i]);

		infer(net, p);

		printf(" output: ");
		for (i = 0; i < OUTPUT_NEURONS; i++) {
			fixtoa(net->output[i], buf, 4);
			printf("%s ", buf);
		}
	}
	printf("\n");
}

void train(struct neural_net_s *net)
{
	int i, j, p, q, r;
	int rnd_index[N_PATTERNS];
	int epoch;
	fixed_t rnd, error, acc;
	fixed_t hidden_delta[HIDDEN_NEURONS];
	fixed_t output_delta[OUTPUT_NEURONS];
	fixed_t diff_hidden_weights[(INPUT_NEURONS+1) * HIDDEN_NEURONS];
	fixed_t diff_output_weights[(HIDDEN_NEURONS+1) * OUTPUT_NEURONS];
	char buf[30];

	for (p = 0; p < N_PATTERNS; p++)
		rnd_index[p] = p;

	// init hidden_weights and diff_hidden_weights
	for (i = 0; i < HIDDEN_NEURONS; i++) {
		for (j = 0; j <= INPUT_NEURONS; j++) {
			diff_hidden_weights[j * HIDDEN_NEURONS + i] = fix_val(0.0);
			rnd = fix_div(fix_val((float) (random() % 100)), fix_val(100.0));
			net->weights.hidden_weights[j * HIDDEN_NEURONS + i] = fix_mul(fix_mul(fix_val(2.0), (rnd - fix_val(0.5))), fix_val(INITIAL_WEIGHT_MAX));
		}
	}

	// init output_weights and diff_output_weights
	for (i = 0; i < OUTPUT_NEURONS; i++) {
		for (j = 0; j <= HIDDEN_NEURONS; j++) {
			diff_output_weights[j * OUTPUT_NEURONS + i] = fix_val(0.0);
			rnd = fix_div(fix_val((float) (random() % 100)), fix_val(100.0));
			net->weights.output_weights[j * OUTPUT_NEURONS + i] = fix_mul(fix_mul(fix_val(2.0), (rnd - fix_val(0.5))), fix_val(INITIAL_WEIGHT_MAX));
		}
	}
	printf("\ninitial outputs (not trained):");
	show_training(net);

	for (epoch = 1; epoch < EPOCHS_MAX; epoch++) {
		// randomize training pattern order
		for (p = 0; p < N_PATTERNS; p++) {
			q = random() % N_PATTERNS;
			r = rnd_index[p];
			rnd_index[p] = rnd_index[q];
			rnd_index[q] = r;
		}
		error = fix_val(0.0);

		// cycle through training patterns
		for (q = 0; q < N_PATTERNS; q++) {
			p = rnd_index[q];

			// feedforward: compute hidden layer activations
			for (i = 0; i < HIDDEN_NEURONS; i++) {
				acc = net->weights.hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i];
				for (j = 0; j < INPUT_NEURONS; j++)
					acc += fix_mul(fix_val((float) net->input[p * INPUT_NEURONS + j]), net->weights.hidden_weights[j * HIDDEN_NEURONS + i]);
				net->hidden[i] = sigmoid(acc);
			}

			// feedforward: compute output layer activation errors
			for (i = 0; i < OUTPUT_NEURONS; i++) {
				acc = net->weights.output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i];
				for (j = 0; j < HIDDEN_NEURONS; j++)
					acc += fix_mul(net->hidden[j], net->weights.output_weights[j * OUTPUT_NEURONS + i]);
				net->output[i] = sigmoid(acc);
				output_delta[i] = fix_mul(fix_mul((fix_val((float) net->target[p * OUTPUT_NEURONS + i]) - net->output[i]), net->output[i]), (fix_val(1.0) - net->output[i]));
				error += fix_mul(fix_mul(fix_val(0.5), (fix_val((float) net->target[p * OUTPUT_NEURONS + i]) - net->output[i])), (fix_val((float) net->target[p * OUTPUT_NEURONS + i]) - net->output[i]));	
			}

			// backpropagation of errors to hidden layer
			for (i = 0; i < HIDDEN_NEURONS; i++) {
				acc = fix_val(0.0);
				for (j = 0; j < OUTPUT_NEURONS; j++)
					acc += fix_mul(net->weights.output_weights[i * OUTPUT_NEURONS + j], output_delta[j]);
				hidden_delta[i] = fix_mul(fix_mul(acc, net->hidden[i]), (fix_val(1.0) - net->hidden[i]));
			}

			// update weights (input to hidden)
			for (i = 0; i < HIDDEN_NEURONS; i++) {
				diff_hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i] = fix_mul(fix_val(ETA), hidden_delta[i]) + fix_mul(fix_val(ALPHA), diff_hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i]);
				net->weights.hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i] += diff_hidden_weights[INPUT_NEURONS * HIDDEN_NEURONS + i];
				for (j = 0; j < INPUT_NEURONS; j++) {
					diff_hidden_weights[j * HIDDEN_NEURONS + i] = fix_mul(fix_mul(fix_val(ETA), fix_val((float) net->input[p * INPUT_NEURONS + j])), hidden_delta[i]) + fix_mul(fix_val(ALPHA), diff_hidden_weights[j * HIDDEN_NEURONS + i]);
					net->weights.hidden_weights[j * HIDDEN_NEURONS + i] += diff_hidden_weights[j * HIDDEN_NEURONS + i] ;
				}
			}

			// update weights (hidden to output)
			for (i = 0; i < OUTPUT_NEURONS; i++) {
				diff_output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i] = fix_mul(fix_val(ETA), output_delta[i]) + fix_mul(fix_val(ALPHA), diff_output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i]);
				net->weights.output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i] += diff_output_weights[HIDDEN_NEURONS * OUTPUT_NEURONS + i];
				for (j = 0; j < HIDDEN_NEURONS; j++) {
					diff_output_weights[j * OUTPUT_NEURONS + i] = fix_mul(fix_mul(fix_val(ETA), net->hidden[j]), output_delta[i]) + fix_mul(fix_val(ALPHA), diff_output_weights[j * OUTPUT_NEURONS + i]);
					net->weights.output_weights[j * OUTPUT_NEURONS + i] += diff_output_weights[j * OUTPUT_NEURONS + i];
				}
			}
		}

		if ((epoch % STATUS_CYCLES) == 0) {
			fixtoa(error, buf, 6);
			printf("\nepoch: %d, error: %s", epoch, buf);
			show_training(net);
		}

		// if error rate is smaller than EPSILON, we are ok!
		if (error < fix_val(EPSILON)) break;
	}

	fixtoa(error, buf, 6);
	printf("\n\ntraining set solved, %d epochs (error: %s)\n", epoch, buf);
}

int main(void)
{
	char input[N_PATTERNS * INPUT_NEURONS] = {
		1, 1, 1, 1, 1, 1, 0,	// 0
		0, 1, 1, 0, 0, 0, 0,	// 1
		1, 1, 0, 1, 1, 0, 1,	// 2
		1, 1, 1, 1, 0, 0, 1,	// 3
		0, 1, 1, 0, 0, 1, 1,	// 4
		1, 0, 1, 1, 0, 1, 1,	// 5
		1, 0, 1, 1, 1, 1, 1,	// 6
		1, 1, 1, 0, 0, 0, 0,	// 7
		1, 1, 1, 1, 1, 1, 1,	// 8
		1, 1, 1, 0, 0, 1, 1,	// 9
		1, 1, 1, 0, 1, 1, 1,	// A
		0, 0, 1, 1, 1, 1, 1,	// B
		1, 0, 0, 1, 1, 1, 0,	// C
		0, 1, 1, 1, 1, 0, 1,	// D
		1, 0, 0, 1, 1, 1, 1,	// E
		1, 0, 0, 0, 1, 1, 1		// F
	};

	char target[N_PATTERNS * OUTPUT_NEURONS] = {
		0, 0, 0, 0,
		0, 0, 0, 1,
		0, 0, 1, 0,
		0, 0, 1, 1,
		0, 1, 0, 0,
		0, 1, 0, 1,
		0, 1, 1, 0,
		0, 1, 1, 1,
		1, 0, 0, 0,
		1, 0, 0, 1,
		1, 0, 1, 0,
		1, 0, 1, 1,
		1, 1, 0, 0,
		1, 1, 0, 1,
		1, 1, 1, 0,
		1, 1, 1, 1
	};

	struct neural_net_s neural_network;

	load(&neural_network, input, target);
	train(&neural_network);
	show_training(&neural_network);

	return 0;
}