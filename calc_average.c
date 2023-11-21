#include <stdio.h>

int main(int argc, char *argv[]) {
	unsigned long long sum = 0;
	unsigned long long count = 0;
	    if (argc != 2) {
		            printf("Usage: %s <filename>\n", argv[0]);
			            return 1;
				        }

	        FILE *file = fopen(argv[1], "r");
		    if (file == NULL) {
			            perror("Error opening file");
				            return 1;
					        }

		        int number;
			    while (fscanf(file, "%d", &number) != EOF) {
				sum += number;
				count++;
			    }

			        fclose(file);
				printf("Average %f \n", (double) sum / count);
				    return 0;
}
