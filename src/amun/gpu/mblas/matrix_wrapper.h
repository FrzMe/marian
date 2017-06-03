#pragma once
#include "matrix.h"

namespace amunmt {
namespace GPU {
namespace mblas {

template <typename T>
class TMatrixWrapper
{
public:
  TMatrixWrapper(const TMatrix<T> &matrix)
  {
    dim_[0] = matrix.dim(0);
    dim_[1] = matrix.dim(1);
    dim_[2] = matrix.dim(2);
    dim_[3] = matrix.dim(3);
    updateStrides();

    data_ = nullptr;
    dataConst_ = matrix.data();
  }

  TMatrixWrapper(TMatrix<T> &matrix)
  {
    dim_[0] = matrix.dim(0);
    dim_[1] = matrix.dim(1);
    dim_[2] = matrix.dim(2);
    dim_[3] = matrix.dim(3);
    updateStrides();

    data_ = matrix.data();
    dataConst_ = data_;
  }

  TMatrixWrapper(size_t other[SHAPE_SIZE])
  { // test constructor
    dim_[0] = other[0];
    dim_[1] = other[1];
    dim_[2] = other[2];
    dim_[3] = other[3];
    updateStrides();
  }

  __device__ __host__
  size_t dim(size_t i) const
  {  return dim_[i]; }

  __device__ __host__
  size_t size() const
  {
    return size_;
  }

  __device__ __host__
  size_t stride(size_t i) const
  {
    return stride_[i];
  }

  __host__
  void updateStrides()
  {
    stride_[0] = dim_[1];
    stride_[1] = 1;
    stride_[2] = dim_[0] * dim_[1];
    stride_[3] = dim_[0] * dim_[1] * dim_[2];

    size_ = stride_[3] * dim_[3];
  }

  __device__
  T* data()
  {
    assert(data_);
    return data_;
  }

  __device__
  const T* data() const
  {
    assert(dataConst_);
    return dataConst_;
  }

  __device__
  const T &operator[](size_t i) const
  {
    assert(i < size());
    return data()[i];
  }

  __device__
  T &operator[](size_t i)
  {
    assert(i < size());
    return data()[i];
  }

  __device__
  const T &operator[](size_t indices[SHAPE_SIZE]) const
  {
    size_t id = indices2Id(indices);
    assert(id < size());
    return data()[id];
  }

  __device__
  T &operator[](size_t indices[SHAPE_SIZE])
  {
    size_t id = indices2Id(indices);
    assert(id < size());
    return data()[id];
  }

  __device__ __host__
  void id2Indices(size_t id, size_t out[SHAPE_SIZE]) const
  {
    assert(id < size());

    out[3] = id / stride(3);
    id = id % stride(3);

    out[2] = id / stride(2);
    id = id % stride(2);

    out[0] = id / stride(0);
    id = id % stride(0);

    out[1] = id / stride(1);
    id = id % stride(1);
  }

  __device__ __host__
  size_t indices2Id(size_t indices[SHAPE_SIZE]) const
  {
    size_t ind = 0;
    for (size_t i = 0; i < SHAPE_SIZE; ++i) {
      ind += indices[i] * stride(i);
    }
    assert(ind < size());
    return ind;
  }

  std::string Debug() const
  {
    std::stringstream strm;

    strm << "dim=";
    for (size_t i = 0; i < SHAPE_SIZE; ++i) {
      strm << dim_[i] << " ";
    }
    strm << "=" << size_;

    strm << "stride=";
    for (size_t i = 0; i < SHAPE_SIZE; ++i) {
      strm << stride(i) << " ";
    }

    return strm.str();
  }

protected:
  size_t dim_[SHAPE_SIZE];
  size_t stride_[SHAPE_SIZE];
  size_t size_;

  T *data_;
  const T *dataConst_;

};

///////////////////////////////////////////////////////////////////////////////

inline void testidToMatrixInd()
{
  size_t dim[4] = {2, 4, 3, 5};
  TMatrixWrapper<float> matrix(dim);

  std::cerr << "matrix=" << matrix.Debug() << std::endl;

  for (size_t i = 0; i < matrix.size(); ++i) {
    matrix.id2Indices(i, dim);

    std::cerr << i << "=";
    for (size_t j = 0; j < SHAPE_SIZE; ++j) {
      std::cerr << " " << dim[j];
    }

    std::cerr << " = " << matrix.indices2Id(dim);
    std::cerr << std::endl;
  }
}

}
}
}