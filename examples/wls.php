<?php

use Numerus\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
    [1.0, 4.0],
]);

$response = Matrix::fromRows([
    [2.0],
    [2.9],
    [4.2],
    [4.8],
]);

// Larger weights give observations more influence in the weighted objective.
$weights = [1.0, 2.0, 2.0, 1.0];
$coefficients = $design->weightedLeastSquares($response, $weights);

printf("intercept: %.6f\n", $coefficients->get(0, 0));
printf("slope: %.6f\n", $coefficients->get(1, 0));
