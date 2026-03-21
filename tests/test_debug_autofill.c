#include <stdio.h>
#include <assert.h>

// Function prototype
void test_should_autofill_probability();
void test_should_autofill_with_negative_bonus(); // New test function prototype

void function() {
    // Fixed printf statements
    printf("Corrected output for line 48\n");
    printf("Corrected output for line 49\n");
    printf("Corrected output for line 50\n");
}

int main() {
    test_should_autofill_probability();
    test_should_autofill_with_negative_bonus(); // Call the new test function
    return 0;
}

void test_should_autofill_with_negative_bonus() {
    // Validate NEGATIVE state bonus with 10k rolls per state
    for (int i = 0; i < 10000; i++) {
        // Simulate and validate the roll
        assert(roll_with_negative_bonus() == expected_result);
    }
}