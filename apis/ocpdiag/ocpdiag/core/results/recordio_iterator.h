// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_
#define OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_

#include <memory>
#include <optional>
#include <string>

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
  // If file_path is left unset, it will construct an invalid iterator.
  RecordIoIterator(std::optional<absl::string_view> file_path = std::nullopt) {
    if (!file_path.has_value()) return;
    reader_ = std::make_unique<riegeli::RecordReader<riegeli::FdReader<>>>(
        riegeli::FdReader{*file_path});
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

// RecordIoContainer defines a container of record io that you can iterate
// through.
//
// Example:
//   for (MessageType msg : RecordIoContainer<MessageType>(path)) {...}
template <class Message>
class RecordIoContainer {
 public:
  // The iterator allows us to be used as a container in range-based for loops.
  using const_iterator = RecordIoIterator<Message>;

  // The container will read from the given file_path.
  RecordIoContainer(absl::string_view file_path) : file_path_(file_path) {}

  absl::string_view file_path() const { return file_path_; }

  const_iterator begin() const { return RecordIoIterator<Message>(file_path_); }
  const_iterator end() const { return RecordIoIterator<Message>(); }

 private:
  std::string file_path_;
};

}  // namespace ocpdiag::results

#endif  // OCPDIAG_CORE_RESULTS_RECORDIO_ITERATOR_H_
