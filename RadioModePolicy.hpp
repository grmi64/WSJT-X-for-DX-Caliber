#ifndef RADIO_MODE_POLICY_HPP
#define RADIO_MODE_POLICY_HPP

#include "Transceiver/Transceiver.hpp"

class RadioModePolicy
{
public:
  using Mode = Transceiver::MODE;
  enum class State { Managed, Preparing, Yielded };

  State state () const { return state_; }
  bool ready () const { return state_ == State::Managed; }
  bool yielded () const { return state_ == State::Yielded; }
  bool preparation_failed () const { return preparation_failed_; }
  bool has_prepared () const { return prepared_; }
  bool waiting_for_hardware_split () const { return split_recovery_pending_; }

  void configure (Mode mode)
  {
    if (mode == desired_) return;
    desired_ = mode;
    split_recovery_pending_ = false;
    split_recovery_mode_ = Transceiver::UNK;
    prepared_ = false;
    preparation_failed_ = false;
    if (mode == Transceiver::UNK)
      {
        state_ = State::Managed;
        preparation_failed_ = false;
      }
  }

  void prepare (unsigned sequence)
  {
    if (desired_ == Transceiver::UNK) return;
    split_recovery_pending_ = false;
    prepared_ = true;
    sequence_ = sequence;
    state_ = State::Preparing;
    preparation_failed_ = false;
  }

  void observe (Transceiver::ModeStatus const& status, unsigned sequence)
  {
    if (desired_ == Transceiver::UNK || sequence < sequence_) return;
    if (split_recovery_pending_)
      {
        if (status.kind == Transceiver::ModeStatusKind::Observed)
          split_recovery_mode_ = status.mode;
        else
          discard_split_recovery_readback ();
        return;
      }
    switch (status.kind)
      {
      case Transceiver::ModeStatusKind::Preparing:
        if (yielded ()) return;
        prepare (sequence);
        break;
      case Transceiver::ModeStatusKind::Unavailable:
        if (state_ == State::Preparing)
          {
            state_ = State::Yielded;
            preparation_failed_ = true;
          }
        break;
      case Transceiver::ModeStatusKind::Applied:
      case Transceiver::ModeStatusKind::Observed:
        if (!prepared_ || status.mode == Transceiver::UNK) return;
        if (status.mode != desired_)
          {
            state_ = State::Yielded;
            preparation_failed_ = false;
          }
        else if (!yielded ())
          {
            state_ = State::Managed;
            preparation_failed_ = false;
          }
        break;
      }
  }

  void hold_for_hardware_split ()
  {
    if (desired_ == Transceiver::UNK) return;
    split_recovery_pending_ = true;
    discard_split_recovery_readback ();
    state_ = State::Yielded;
    preparation_failed_ = true;
  }

  void discard_split_recovery_readback () { split_recovery_mode_ = Transceiver::UNK; }

  void confirm_idle_simplex (Mode mode)
  {
    if (!split_recovery_pending_ || mode != desired_ || mode != split_recovery_mode_) return;
    split_recovery_pending_ = false;
    state_ = State::Managed;
    prepared_ = true;
    preparation_failed_ = false;
  }

private:
  Mode desired_ = Transceiver::UNK;
  State state_ = State::Managed;
  unsigned sequence_ = 0;
  bool prepared_ = false;
  bool preparation_failed_ = false;
  bool split_recovery_pending_ = false;
  Mode split_recovery_mode_ = Transceiver::UNK;
};

#endif
