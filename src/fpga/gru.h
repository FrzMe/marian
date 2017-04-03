#pragma once

namespace amunmt {
namespace FPGA {

template <class Weights>
class SlowGRU {
public:
  SlowGRU(const OpenCLInfo &openCLInfo, const Weights& model)
  : openCLInfo_(openCLInfo)
  , w_(model)
  , RU_(openCLInfo)
  , H_(openCLInfo)

  , Temp1_(openCLInfo)
  , Temp2_(openCLInfo)
  {
  }

  size_t GetStateLength() const {
    return w_.U_.dim(0);
  }

  void GetNextState(mblas::Matrix& NextState,
                    const mblas::Matrix& State,
                    const mblas::Matrix& Context) const
  {
    using namespace mblas;

    const size_t cols = GetStateLength();

    // @TODO: Optimization
    // @TODO: Launch streams to perform GEMMs in parallel
    // @TODO: Join matrices and perform single GEMM --------
    Prod(RU_, Context, w_.W_);
    Prod(H_,  Context, w_.Wx_);
    // -----------------------------------------------------

    // @TODO: Join matrices and perform single GEMM --------
    Prod(Temp1_, State, w_.U_);
    Prod(Temp2_, State, w_.Ux_);
    std::cerr << "Temp2_=" << Temp2_.Debug(1) << std::endl;
    // -----------------------------------------------------

    std::cerr << "1RU_=" << RU_.Debug(1) << std::endl;
    std::cerr << "w_.B_=" << w_.B_.Debug(1) << std::endl;
    BroadcastVecAdd(RU_, w_.B_); // Broadcasting row-wise
    std::cerr << "2RU_=" << RU_.Debug(1) << std::endl;

  }

protected:
  const OpenCLInfo &openCLInfo_;
  // Model matrices
  const Weights& w_;

  // reused to avoid allocation
  mutable mblas::Matrix RU_;
  mutable mblas::Matrix H_;

  mutable mblas::Matrix Temp1_;
  mutable mblas::Matrix Temp2_;

};



///////////////////////////////////////////////////////////////////////////////////////////////
template <class Weights>
class FastGRU {
public:
  FastGRU(const OpenCLInfo &openCLInfo, const Weights& model)
  : openCLInfo_(openCLInfo)
  , w_(model)
  , WWx_(openCLInfo)
  , UUx_(openCLInfo)
  , RUH_(openCLInfo)
  , Temp_(openCLInfo)
  {
    using namespace mblas;

    Transpose(WWx_, w_.W_);
    std::cerr << std::endl;
    std::cerr << "w_.W_=" << w_.W_.Debug(1) << std::endl;
    std::cerr << "1WWx_=" << WWx_.Debug(1) << std::endl;

    Matrix WxT(openCLInfo);
    Transpose(WxT, w_.Wx_);
    std::cerr << "w_.Wx_=" << w_.Wx_.Debug(1) << std::endl;
    std::cerr << "WxT=" << WxT.Debug(1) << std::endl;

    Concat(WWx_, WxT);
    std::cerr << "2WWx_=" << WWx_.Debug(1) << std::endl;

    Transpose(WWx_);
    std::cerr << "3WWx_=" << WWx_.Debug(1) << std::endl;

    Transpose(UUx_, w_.U_);

    Matrix UxT(openCLInfo);
    Transpose(UxT, w_.Ux_);

    Concat(UUx_, UxT);
    Transpose(UUx_);

  }

  size_t GetStateLength() const {
    return w_.U_.dim(0);
  }

  void GetNextState(mblas::Matrix& NextState,
                    const mblas::Matrix& State,
                    const mblas::Matrix& Context) const
  {
    using namespace mblas;

    std::cerr << std::endl;
    std::cerr << "1RUH_=" << RUH_.Debug(1) << std::endl;
    std::cerr << "Context=" << Context.Debug(1) << std::endl;
    std::cerr << "WWx_" << WWx_.Debug(1) << std::endl;

    Prod(RUH_, Context, WWx_);

    std::cerr << "2RUH_=" << RUH_.Debug(1) << std::endl;

    if (w_.Gamma_1_) {
      Normalization(RUH_, RUH_, w_.Gamma_1_, 1e-9);
    }
    std::cerr << "3RUH_=" << RUH_.Debug(1) << std::endl;
    std::cerr << "State=" << State.Debug(1) << std::endl;
    std::cerr << "UUx_" << UUx_.Debug(1) << std::endl;

    Prod(Temp_, State, UUx_);
    std::cerr << "Temp_=" << Temp_.Debug(1) << std::endl;

    if (w_.Gamma_2_) {
      Normalization(Temp_, Temp_, w_.Gamma_2_, 1e-9);
    }

    ElementwiseOps(NextState, State, RUH_, Temp_);

  }

  void ElementwiseOps(mblas::Matrix& NextState,
                      const mblas::Matrix& State,
                      const mblas::Matrix& RUH,
                      const mblas::Matrix& Temp) const
  {
    const uint rows = State.dim(0) * State.dim(2) * State.dim(3);
    const uint cols = State.dim(1);

    NextState.Resize(State.dim(0) * State.dim(3), cols, State.dim(2), 1);
    //std::cerr << "NextState=" << NextState.Debug() << std::endl;

    mblas::ElementwiseOps(NextState, State, RUH, Temp, w_.B_, w_.Bx1_, w_.Bx2_, rows, cols);

  }

protected:
  const OpenCLInfo &openCLInfo_;
  // Model matrices
  const Weights& w_;

  // reused to avoid allocation
  mutable mblas::Matrix WWx_;
  mutable mblas::Matrix UUx_;

  mutable mblas::Matrix RUH_;
  mutable mblas::Matrix Temp_;

};

template<class T>
using GRU = SlowGRU<T>;

}
}
