/**
 * @file coverage_driver.c
 * @brief Runs focused native test suites against one instrumented source build.
 *
 * @details The coverage workflow renames each suite's main function and calls
 * it from this driver so all native source translation units are linked into
 * one instrumented executable. This lets gcovr report zero-covered source
 * files as well as files exercised by the selected suites.
 */

#include <stdio.h>

int numerus_coverage_storage_main(void);
int numerus_coverage_matrix_property_main(void);
int numerus_coverage_matrix_adversarial_main(void);
int numerus_coverage_statistical_property_main(void);

int main(void)
{
    if (numerus_coverage_storage_main() != 0) {
        return 1;
    }
    if (numerus_coverage_matrix_property_main() != 0) {
        return 1;
    }
    if (numerus_coverage_matrix_adversarial_main() != 0) {
        return 1;
    }
    if (numerus_coverage_statistical_property_main() != 0) {
        return 1;
    }

    puts("Instrumented native coverage suites passed.");
    return 0;
}
