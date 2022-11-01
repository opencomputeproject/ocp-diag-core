// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_
#define OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_

#include <memory>
#include <optional>

#include "absl/log/check.h"
#include "riegeli/bytes/fd_reader.h"
#include "riegeli/records/record_reader.h"

namespace ocpdiag::results {

// Satisfies the interface for range-based for loops in C++, to allow you to
// iterate through arbitrary protos in a recordio file. It crashes if errors are
// encountered, so this is not suitable for production code. It is intended for
// unit tests only.
template <class MessageType>
class RecordIoIterator {
 public:
  // Constructs a new iterator, pointing to the first proto message (if any).
  // If file_path is empty, it will construct an invalid iterator.
  RecordIoIterator(absl::string_view file_path = "") {
    if (file_path.empty()) return;
    reader_ = std::make_unique<riegeli::RecordReader<riegeli::FdReader<>>>(
        riegeli::FdReader{file_path});
    ++(*this);  // advance ourselves so we always start on the first item
  }

  // Dereferences the iterator.
  MessageType &operator*() { return message_; }
  MessageType *operator->() { return &message_; }

  // Advances the iterator.
  RecordIoIterator &operator++() {
    if (!reader_->ReadRecord(message_)) {
      CHECK_OK(reader_->status()) << "Failed while reading recordio";
      reader_.reset();  // indicate EOF
    }
    return *this;
  }

  // The boolean operator can also be used to tell if the iterator still has
  // data left to consume.
  operator bool() const { return reader_ != nullptr; }

  // We can only compare valid iterator vs. invalid, but we can't tell the
  // difference between two valid iterators.
  bool operator!=(const RecordIoIterator &other) const {
    return (bool)*this != (bool)other;
  }

 private:
  std::unique_ptr<riegeli::RecordReader<riegeli::FdReader<>>> reader_;
  MessageType message_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_
