require "bundler/gem_tasks"
require "rake/testtask"
import "ext/ffi/fasttext/Rakefile"

namespace :fasttext do
  desc "build fasttext"
  task :compile do
    ::Rake::Task[:compile_fasttext].invoke
  end
end

Rake::TestTask.new(:test) do |t|
  t.libs << "test"
  t.libs << "lib"
  t.test_files = FileList["test/**/*_test.rb"]
end
Rake::Task[:test].prerequisites << "fasttext:compile"

task :default => :test
