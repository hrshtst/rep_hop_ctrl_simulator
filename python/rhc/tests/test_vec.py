from __future__ import annotations

import numpy as np
import pytest

import rhc


def test_create_size_is_zero_initialized():
    v = rhc.Vec(3)
    assert len(v) == 3
    assert v.size == 3
    assert v[0] == 0.0
    assert v[1] == 0.0
    assert v[2] == 0.0


def test_from_sequence():
    v = rhc.Vec([1.0, 2.0, 3.0])
    assert len(v) == 3
    assert v[0] == 1.0
    assert v[2] == 3.0


def test_setitem():
    v = rhc.Vec(2)
    v[0] = 1.5
    v[1] = -2.0
    assert v[0] == 1.5
    assert v[1] == -2.0


def test_getitem_out_of_range_raises():
    v = rhc.Vec(2)
    with pytest.raises(IndexError):
        _ = v[2]


def test_to_numpy_copies():
    v = rhc.Vec([1.0, 2.0, 3.0])
    a = v.to_numpy()
    assert isinstance(a, np.ndarray)
    np.testing.assert_allclose(a, [1.0, 2.0, 3.0])
    # A copy: mutating the array does not touch the Vec.
    a[0] = 99.0
    assert v[0] == 1.0


def test_add_and_sub():
    a = rhc.Vec([1.0, 2.0])
    b = rhc.Vec([3.0, 4.0])
    np.testing.assert_allclose((a + b).to_numpy(), [4.0, 6.0])
    np.testing.assert_allclose((a - b).to_numpy(), [-2.0, -2.0])


def test_scalar_mul():
    v = rhc.Vec([1.0, -2.0])
    np.testing.assert_allclose((v * 3.0).to_numpy(), [3.0, -6.0])


def test_dot_and_norm():
    a = rhc.Vec([1.0, 2.0])
    b = rhc.Vec([3.0, 4.0])
    assert a.dot(b) == 11.0
    assert rhc.Vec([3.0, 4.0]).norm() == 5.0


def test_size_mismatch_raises():
    a = rhc.Vec([1.0, 2.0])
    b = rhc.Vec([1.0, 2.0, 3.0])
    with pytest.raises(ValueError):
        _ = a + b


def test_equality():
    assert rhc.Vec([1.0, 2.0]) == rhc.Vec([1.0, 2.0])
    assert rhc.Vec([1.0, 2.0]) != rhc.Vec([1.0, 2.5])
