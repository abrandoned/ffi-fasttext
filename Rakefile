require "bundler/gem_tasks"
require "rake/testtask"
import "ext/ffi/fasttext/Rakefile"

namespace :fasttext do
  desc "build fasttext"
  task :compile do
    ::Rake::Task[:compile_fasttext].invoke
  end
end

Rake::TestTask.new(:spec) do |t|
  t.libs << "spec"
  t.libs << "lib"
  t.test_files = FileList["spec/**/*_spec.rb"]
end
Rake::Task[:spec].prerequisites << "fasttext:compile"

task :default => :spec
