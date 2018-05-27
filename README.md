# FFI::Fasttext

FFI bindings for [Fasttext](https://fasttext.cc/)

If you aren't sure what Fasttext is then read the information provided at the site above or the facebook github profile for the source of the fasttext C++ code [Github:Fasttext](https://github.com/facebookresearch/fastText/)

The FFI bindings are only for using Fasttext predictions in a ruby process after a model has been trained in the console. The model.bin/model.vec files are required to use the prediction probabilities through the `#predict` method of a `::FFI::Fasttext::Predictor` object.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'ffi-fasttext'
```

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install ffi-fasttext

## Usage

Requires `g++` to be installed as it uses the C++ compiler to build the shared object that FFI uses and the executable unless other configuration options for access to the shared object are provided.

The Fasttext C++ model loading code will error out (and cause a hard `exit`) if the wrong model file is used or the model file is not at the specified filename. (so make sure you use the \*.bin file in the initialization)

There is a test training set and model in the spec directory, which should not be relied on for any predictability as it is abbreviated and all trained on "derp" derivations.

```ruby
require "ffi/fasttext"

ft = ::FFI::Fasttext::Predictor.new("spec/model.bin")

ft.predict("derp") # => [["__label__3", 0.4375]] // will output the highest probability label and the associated probability in an array
ft.predict("derp", 3) # => [["__label__3", 0.4375], ["__label__1", 0.396484], ["__label__2", 0.164063]] // will output the top 3 probabilities in an array of arrays
ft.predict("derp", 10) # => [["__label__3", 0.4375], ["__label__1", 0.396484], ["__label__2", 0.164063]] // output the same as above as there are only 3 categories or if probability < 0

ft.destroy! # The prediction model is dynamically allocated in C code and must be released
```

## Development

After checking out the repo, run `bin/setup` to install dependencies. Then, run `rake spec` to run the tests. You can also run `bin/console` for an interactive prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To release a new version, update the version number in `version.rb`, and then run `bundle exec rake release`, which will create a git tag for the version, push git commits and tags, and push the `.gem` file to [rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/abrandoned/ffi-fasttext

## License

The gem is available as open source under the terms of the [MIT License](https://opensource.org/licenses/MIT)
The original source is licensed per Facebook under the terms of the [BSD License](https://github.com/abrandoned/ffi-fasttext/blob/master/vendor/fasttext/LICENSE)
Along with a Patent License from Facebook [Patents](https://github.com/abrandoned/ffi-fasttext/blob/master/vendor/fasttext/PATENTS)
