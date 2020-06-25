require "ffi"
require "ffi/fasttext/version"

module FFI
  module Fasttext
    extend FFI::Library
    ffi_lib_flags :now, :global

    ##
    # ffi-rzmq-core for reference
    #
    # https://github.com/chuckremes/ffi-rzmq-core/blob/master/lib/ffi-rzmq-core/libzmq.rb
    #
    begin
      # bias the library discovery to a path inside the gem first, then
      # to the usual system paths
      gem_base = ::File.join(::File.dirname(__FILE__), '..', '..')
      inside_gem = ::File.join(gem_base, 'ext')
      local_path = ::FFI::Platform::IS_WINDOWS ? ENV['PATH'].split(';') : ENV['PATH'].split(':')
      env_path = [ ENV['FASTTEXT_LIB_PATH'] ].compact
      rbconfig_path = ::RbConfig::CONFIG["libdir"]
      homebrew_path = nil

      # RUBYOPT set by RVM breaks 'brew' so we need to unset it.
      rubyopt = ENV.delete('RUBYOPT')

      begin
        stdout, stderr, status = ::Open3.capture3("brew", "--prefix")
        homebrew_path  = if status.success?
                           "#{stdout.chomp}/lib"
                         else
                           '/usr/local/homebrew/lib'
                         end
      rescue
        # Homebrew doesn't exist
      end

      # Restore RUBYOPT after executing 'brew' above.
      ENV['RUBYOPT'] = rubyopt

      # Search for libfasttext in the following order...
      fasttext_lib_paths =
        if ENV.key?("FASTTEXT_USE_SYSTEM_LIB")
          [inside_gem] + env_path + local_path + [rbconfig_path] + [
           '/usr/local/lib', '/opt/local/lib', homebrew_path, '/usr/lib64'
          ]
        else
          [::File.join(gem_base, "vendor/fasttext")]
        end

      FASTTEXT_LIB_PATHS = fasttext_lib_paths.
        compact.map{|path| "#{path}/libfasttext.#{::FFI::Platform::LIBSUFFIX}"}

      ffi_lib(FASTTEXT_LIB_PATHS + %w{libfasttext})
    rescue LoadError => error
      if FASTTEXT_LIB_PATHS.any? {|path| ::File.file?(::File.join(path)) }
        warn "Unable to load this gem. The libfasttext library exists, but cannot be loaded."
        warn "Set FASTTEXT_LIB_PATH if custom load path is desired"
        warn "If this is Windows:"
        warn "-  Check that you have MSVC runtime installed or statically linked"
        warn "-  Check that your DLL is compiled for #{FFI::Platform::ADDRESS_SIZE} bit"
      else
        warn "Unable to load this gem. The libfasttext library (or DLL) could not be found."
        warn "Set FASTTEXT_LIB_PATH if custom load path is desired"
        warn "If this is a Windows platform, make sure libfasttext.dll is on the PATH."
        warn "If the DLL was built with mingw, make sure the other two dependent DLLs,"
        warn "libgcc_s_sjlj-1.dll and libstdc++6.dll, are also on the PATH."
        warn "For non-Windows platforms, make sure libfasttext is located in this search path:"
        warn FASTTEXT_LIB_PATHS.inspect
      end
      raise error
    end

    attach_function :create, [:string], :pointer
    attach_function :create_from_url, [:string], :pointer
    attach_function :destroy, [:pointer], :void
    attach_function :predict_string_free, [:pointer], :void
    attach_function :predict, [:pointer, :string, :int32_t], :strptr

    class Predictor
      def initialize(model_name)
        @ptr = ::File.exist?(model_name) ? ::FFI::Fasttext.create(model_name) : ::FFI::Fasttext.create_from_url(model_name)
        raise "Error loading model" if @ptr.null?
      end

      def destroy!
        ::FFI::Fasttext.destroy(@ptr) unless @ptr.nil?
        @ptr = nil
      end

      def predict(string, number_of_predictions = 1)
        response_string, pointer = ::FFI::Fasttext.predict(@ptr, string, number_of_predictions)
        return [] unless response_string.size > 0

        response_array = []
        split_responses = response_string.split(" ")
        split_responses.each_slice(2) do |pair|
          next unless pair.first && pair.last

          response_array << [pair.first, pair.last.to_f]
        end

        response_array
      ensure
        ::FFI::Fasttext.predict_string_free(pointer) unless pointer.nil?
      end
    end
  end
end
