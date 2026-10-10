/**
 * @file coverage_driver.c
 * @brief Runs every standalone native test suite against one instrumented build.
 *
 * The coverage workflow renames each suite's main function and links all suites
 * into this driver. Keep the suite list synchronized with the standalone native test files.
 */

#include <stdio.h>

int numerus_coverage_gaussian_log_density_main(void);
int numerus_coverage_gaussian_sampling_main(void);
int numerus_coverage_matrix_adversarial_main(void);
int numerus_coverage_matrix_aggregate_main(void);
int numerus_coverage_matrix_allocation_failure_main(void);
int numerus_coverage_matrix_binary_main(void);
int numerus_coverage_matrix_block_grid_main(void);
int numerus_coverage_matrix_cholesky_main(void);
int numerus_coverage_matrix_close_main(void);
int numerus_coverage_matrix_compare_main(void);
int numerus_coverage_matrix_condition_main(void);
int numerus_coverage_matrix_definiteness_main(void);
int numerus_coverage_matrix_determinant_main(void);
int numerus_coverage_matrix_diagonal_main(void);
int numerus_coverage_matrix_division_main(void);
int numerus_coverage_matrix_elementwise_main(void);
int numerus_coverage_matrix_elimination_main(void);
int numerus_coverage_matrix_exponential_main(void);
int numerus_coverage_matrix_finite_main(void);
int numerus_coverage_matrix_hadamard_main(void);
int numerus_coverage_matrix_inverse_gauss_jordan_main(void);
int numerus_coverage_matrix_inverse_main(void);
int numerus_coverage_matrix_kronecker_main(void);
int numerus_coverage_matrix_ldlt_main(void);
int numerus_coverage_matrix_least_squares_main(void);
int numerus_coverage_matrix_log_determinant_main(void);
int numerus_coverage_matrix_lu_main(void);
int numerus_coverage_matrix_map_main(void);
int numerus_coverage_matrix_materialize_main(void);
int numerus_coverage_matrix_multiply_main(void);
int numerus_coverage_matrix_norm_main(void);
int numerus_coverage_matrix_normalize_main(void);
int numerus_coverage_matrix_null_space_main(void);
int numerus_coverage_matrix_padding_main(void);
int numerus_coverage_matrix_permutation_main(void);
int numerus_coverage_matrix_polynomial_main(void);
int numerus_coverage_matrix_power_main(void);
int numerus_coverage_matrix_property_main(void);
int numerus_coverage_matrix_pseudoinverse_main(void);
int numerus_coverage_matrix_qr_main(void);
int numerus_coverage_matrix_rank_main(void);
int numerus_coverage_matrix_repeat_main(void);
int numerus_coverage_matrix_reshape_main(void);
int numerus_coverage_matrix_scalar_division_main(void);
int numerus_coverage_matrix_selection_main(void);
int numerus_coverage_matrix_slice_main(void);
int numerus_coverage_matrix_solve_main(void);
int numerus_coverage_matrix_spaces_main(void);
int numerus_coverage_matrix_spectral_main(void);
int numerus_coverage_matrix_statistical_workflow_main(void);
int numerus_coverage_matrix_statistics_main(void);
int numerus_coverage_matrix_structural_predicate_main(void);
int numerus_coverage_matrix_svd_main(void);
int numerus_coverage_matrix_symmetric_eigen_main(void);
int numerus_coverage_matrix_symmetric_functions_main(void);
int numerus_coverage_matrix_system_main(void);
int numerus_coverage_matrix_main(void);
int numerus_coverage_matrix_trace_main(void);
int numerus_coverage_matrix_triangular_solve_main(void);
int numerus_coverage_matrix_unary_main(void);
int numerus_coverage_matrix_vector_constructor_main(void);
int numerus_coverage_matrix_weighted_least_squares_main(void);
int numerus_coverage_numeric_stable_functions_main(void);
int numerus_coverage_optimizer_main(void);
int numerus_coverage_rng_sampling_main(void);
int numerus_coverage_rng_state_main(void);
int numerus_coverage_rng_variates_main(void);
int numerus_coverage_statistical_property_main(void);
int numerus_coverage_storage_main(void);

int main(void)
{
    if (numerus_coverage_gaussian_log_density_main() != 0) {
        fprintf(stderr, "Coverage suite failed: gaussian_log_density\n");
        return 1;
    }
    if (numerus_coverage_gaussian_sampling_main() != 0) {
        fprintf(stderr, "Coverage suite failed: gaussian_sampling\n");
        return 1;
    }
    if (numerus_coverage_matrix_adversarial_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_adversarial\n");
        return 1;
    }
    if (numerus_coverage_matrix_aggregate_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_aggregate\n");
        return 1;
    }
    if (numerus_coverage_matrix_allocation_failure_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_allocation_failure\n");
        return 1;
    }
    if (numerus_coverage_matrix_binary_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_binary\n");
        return 1;
    }
    if (numerus_coverage_matrix_block_grid_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_block_grid\n");
        return 1;
    }
    if (numerus_coverage_matrix_cholesky_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_cholesky\n");
        return 1;
    }
    if (numerus_coverage_matrix_close_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_close\n");
        return 1;
    }
    if (numerus_coverage_matrix_compare_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_compare\n");
        return 1;
    }
    if (numerus_coverage_matrix_condition_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_condition\n");
        return 1;
    }
    if (numerus_coverage_matrix_definiteness_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_definiteness\n");
        return 1;
    }
    if (numerus_coverage_matrix_determinant_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_determinant\n");
        return 1;
    }
    if (numerus_coverage_matrix_diagonal_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_diagonal\n");
        return 1;
    }
    if (numerus_coverage_matrix_division_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_division\n");
        return 1;
    }
    if (numerus_coverage_matrix_elementwise_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_elementwise\n");
        return 1;
    }
    if (numerus_coverage_matrix_elimination_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_elimination\n");
        return 1;
    }
    if (numerus_coverage_matrix_exponential_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_exponential\n");
        return 1;
    }
    if (numerus_coverage_matrix_finite_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_finite\n");
        return 1;
    }
    if (numerus_coverage_matrix_hadamard_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_hadamard\n");
        return 1;
    }
    if (numerus_coverage_matrix_inverse_gauss_jordan_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_inverse_gauss_jordan\n");
        return 1;
    }
    if (numerus_coverage_matrix_inverse_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_inverse\n");
        return 1;
    }
    if (numerus_coverage_matrix_kronecker_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_kronecker\n");
        return 1;
    }
    if (numerus_coverage_matrix_ldlt_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_ldlt\n");
        return 1;
    }
    if (numerus_coverage_matrix_least_squares_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_least_squares\n");
        return 1;
    }
    if (numerus_coverage_matrix_log_determinant_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_log_determinant\n");
        return 1;
    }
    if (numerus_coverage_matrix_lu_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_lu\n");
        return 1;
    }
    if (numerus_coverage_matrix_map_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_map\n");
        return 1;
    }
    if (numerus_coverage_matrix_materialize_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_materialize\n");
        return 1;
    }
    if (numerus_coverage_matrix_multiply_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_multiply\n");
        return 1;
    }
    if (numerus_coverage_matrix_norm_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_norm\n");
        return 1;
    }
    if (numerus_coverage_matrix_normalize_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_normalize\n");
        return 1;
    }
    if (numerus_coverage_matrix_null_space_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_null_space\n");
        return 1;
    }
    if (numerus_coverage_matrix_padding_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_padding\n");
        return 1;
    }
    if (numerus_coverage_matrix_permutation_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_permutation\n");
        return 1;
    }
    if (numerus_coverage_matrix_polynomial_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_polynomial\n");
        return 1;
    }
    if (numerus_coverage_matrix_power_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_power\n");
        return 1;
    }
    if (numerus_coverage_matrix_property_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_property\n");
        return 1;
    }
    if (numerus_coverage_matrix_pseudoinverse_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_pseudoinverse\n");
        return 1;
    }
    if (numerus_coverage_matrix_qr_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_qr\n");
        return 1;
    }
    if (numerus_coverage_matrix_rank_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_rank\n");
        return 1;
    }
    if (numerus_coverage_matrix_repeat_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_repeat\n");
        return 1;
    }
    if (numerus_coverage_matrix_reshape_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_reshape\n");
        return 1;
    }
    if (numerus_coverage_matrix_scalar_division_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_scalar_division\n");
        return 1;
    }
    if (numerus_coverage_matrix_selection_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_selection\n");
        return 1;
    }
    if (numerus_coverage_matrix_slice_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_slice\n");
        return 1;
    }
    if (numerus_coverage_matrix_solve_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_solve\n");
        return 1;
    }
    if (numerus_coverage_matrix_spaces_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_spaces\n");
        return 1;
    }
    if (numerus_coverage_matrix_spectral_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_spectral\n");
        return 1;
    }
    if (numerus_coverage_matrix_statistical_workflow_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_statistical_workflow\n");
        return 1;
    }
    if (numerus_coverage_matrix_statistics_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_statistics\n");
        return 1;
    }
    if (numerus_coverage_matrix_structural_predicate_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_structural_predicate\n");
        return 1;
    }
    if (numerus_coverage_matrix_svd_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_svd\n");
        return 1;
    }
    if (numerus_coverage_matrix_symmetric_eigen_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_symmetric_eigen\n");
        return 1;
    }
    if (numerus_coverage_matrix_symmetric_functions_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_symmetric_functions\n");
        return 1;
    }
    if (numerus_coverage_matrix_system_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_system\n");
        return 1;
    }
    if (numerus_coverage_matrix_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix\n");
        return 1;
    }
    if (numerus_coverage_matrix_trace_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_trace\n");
        return 1;
    }
    if (numerus_coverage_matrix_triangular_solve_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_triangular_solve\n");
        return 1;
    }
    if (numerus_coverage_matrix_unary_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_unary\n");
        return 1;
    }
    if (numerus_coverage_matrix_vector_constructor_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_vector_constructor\n");
        return 1;
    }
    if (numerus_coverage_matrix_weighted_least_squares_main() != 0) {
        fprintf(stderr, "Coverage suite failed: matrix_weighted_least_squares\n");
        return 1;
    }
    if (numerus_coverage_numeric_stable_functions_main() != 0) {
        fprintf(stderr, "Coverage suite failed: numeric_stable_functions\n");
        return 1;
    }
    if (numerus_coverage_optimizer_main() != 0) {
        fprintf(stderr, "Coverage suite failed: optimizer\n");
        return 1;
    }
    if (numerus_coverage_rng_sampling_main() != 0) {
        fprintf(stderr, "Coverage suite failed: rng_sampling\n");
        return 1;
    }
    if (numerus_coverage_rng_state_main() != 0) {
        fprintf(stderr, "Coverage suite failed: rng_state\n");
        return 1;
    }
    if (numerus_coverage_rng_variates_main() != 0) {
        fprintf(stderr, "Coverage suite failed: rng_variates\n");
        return 1;
    }
    if (numerus_coverage_statistical_property_main() != 0) {
        fprintf(stderr, "Coverage suite failed: statistical_property\n");
        return 1;
    }
    if (numerus_coverage_storage_main() != 0) {
        fprintf(stderr, "Coverage suite failed: storage\n");
        return 1;
    }

    puts("All instrumented native test suites passed.");
    return 0;
}
