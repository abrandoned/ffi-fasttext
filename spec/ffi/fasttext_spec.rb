require 'spec_helper'
require 'pry'

describe ::FFI::Fasttext::Predictor do
  describe "API" do
    let(:filename) { ::File.join(::File.dirname(__FILE__), "..", "model.bin") }

    it "validates #predict present" do
      ::FFI::Fasttext::Predictor.new(filename).must_respond_to :predict
    end

    it "will not initialize without valid model file" do
      error = lambda { ::FFI::Fasttext::Predictor.new("derpderp") }.must_raise(RuntimeError)
      error.message.must_match(/error loading model/i)
    end

    it "will output an array with matches on #predict" do
      ::FFI::Fasttext::Predictor.new(filename).predict("derp").must_be_instance_of Array
    end

    it "will include an array of arrays with predictions of category and probability when found" do
      predictions = ::FFI::Fasttext::Predictor.new(filename).predict("derp", 10)
      predictions.must_be_instance_of Array
      predictions.size.must_be :>, 1

      predictions.each do |prediction|
        prediction.must_be_instance_of Array
        prediction.size.must_equal 2
        prediction.first.must_be_instance_of String
        prediction.last.must_be_instance_of Float
      end
    end

    it "will include an array of arrays with predictions of category and probability when found and default to 1" do
      predictions = ::FFI::Fasttext::Predictor.new(filename).predict("derp")
      predictions.must_be_instance_of Array
      predictions.size.must_equal 1

      predictions.each do |prediction|
        prediction.must_be_instance_of Array
        prediction.size.must_equal 2
        prediction.first.must_be_instance_of String
        prediction.last.must_be_instance_of Float
      end
    end
  end
end
