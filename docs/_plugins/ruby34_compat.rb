# Ruby 3.2+ removed Object#tainted? and Object#taint, which Liquid 4.x still uses.
# This shim restores them as no-ops so Jekyll can run on Ruby 3.2+.
if RUBY_VERSION >= "3.2"
  module Kernel
    def tainted?
      false
    end

    def taint
      self
    end
  end
end
