#include <ruby.h>
#include <apt-pkg/deblistparser.h>
#include <apt-pkg/debversion.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/init.h>

using namespace std;

/* from '<ruby/dl.h>' */
#define INT2BOOL(x)  ((x) ? Qtrue : Qfalse)

/* function prototypes */
extern "C" void Init_apt_pkg();
/* String functions */
static VALUE uri_to_filename(VALUE self, VALUE uri);
static VALUE time_to_str(VALUE self, VALUE secondes);
static VALUE size_to_str(VALUE self, VALUE size);
static VALUE string_to_bool(VALUE self, VALUE text);
static VALUE check_domain_list(VALUE self, VALUE host, VALUE list);
/* Versioning */
static VALUE cmp_version(VALUE self, VALUE pkg_version_a, VALUE pkg_version_b);
static VALUE check_dep(VALUE self, VALUE pkg_version_a, VALUE op, VALUE pkg_version_b);
static VALUE upstream_version(VALUE self, VALUE ver);

/*
 * call-seq: cmp_version(pkg_version_a, pkg_version_b) -> int
 *
 * Compare the given versions.
 *
 * Return a strictly negative value if 'a' is smaller than 'b', 0 if they
 * are equal, and a strictly positive value if 'a' is larger than 'b'.
 *
 *   Debian::AptPkg.cmp_version('1.1', '1.0') # => 1
 *   Debian::AptPkg.cmp_version('1.0', '1.0') # => 0
 *   Debian::AptPkg.cmp_version('1.0', '1.1') # => -1
 *
 **/
static
VALUE cmp_version(VALUE self, VALUE pkg_version_a, VALUE pkg_version_b) {
	int res = debVS.CmpVersion(StringValuePtr(pkg_version_a), StringValuePtr(pkg_version_b));
	return INT2FIX(res);
}

/*
 * call-seq: check_dep(pkg_version_a, op, pkg_version_b) -> bool
 *
 * Compare the given versions with an operator.
 *
 * Params:
 *
 * +pkg_version_a+:: Version to compare from.
 * +op+:: See operators const or string (<, >, <=, >=, =)
 * +pkg_version_b+:: Version to compare to.
 *
 *   Debian::AptPkg.check_dep('1', '<', '2') # => true
 *   Debian::AptPkg.check_dep('1', Debian::AptPkg::NOT_EQUALS, '2') # => true
 *
 **/
static
VALUE check_dep(VALUE self, VALUE pkg_version_a, VALUE cmp_type, VALUE pkg_version_b) {
	unsigned int op = 0;
	if (TYPE(cmp_type) == T_FIXNUM) {
		op = NUM2INT(cmp_type);
	} else {
		const char *op_str = StringValuePtr(cmp_type);
		if (strcmp(op_str, ">") == 0) op_str = ">>";
		if (strcmp(op_str, "<") == 0) op_str = "<<";
		if (*debListParser::ConvertRelation(op_str, op) != 0)
		{
			rb_raise(rb_eArgError, "Bad comparison operation");
			return 0;
		}
	}
	int res = debVS.CheckDep(StringValuePtr(pkg_version_a), op, StringValuePtr(pkg_version_b));
	return INT2BOOL(res);
}

/*
 * call-seq: uri_to_filename(uri) -> string
 *
 * Return a filename which can be used to store the file.
 *
 *   Debian::AptPkg.uri_to_filename('http://debian.org/index.html') # => 'debian.org_index.html'
 *
 **/
static
VALUE uri_to_filename(VALUE self, VALUE uri) {
	return rb_str_new2(URItoFileName(StringValuePtr(uri)).c_str());
}

/*
 * call-seq: upstream_version(ver) -> string
 *
 * Return the upstream version for the Debian package version given by version.
 *
 *   Debian::AptPkg.upstream_version('3.4.15-1+b1') # => '3.4.15'
 *
 **/
static
VALUE upstream_version(VALUE self, VALUE ver) {
	return rb_str_new2(debVS.UpstreamVersion(StringValuePtr(ver)).c_str());
}

/*
 * call-seq: time_to_str(secondes) -> string
 *
 * Format a given duration in human-readable manner.
 *
 *   Debian::AptPkg.time_to_str(3601) # => '1h 0min 1s'
 *
 **/
static
VALUE time_to_str(VALUE self, VALUE secondes) {
	return rb_str_new2(TimeToStr(NUM2INT(secondes)).c_str());
}

/*
 * call-seq: size_to_str(size) -> string
 *
 * Return a string describing the size in a human-readable manner.
 *
 *   Debian::AptPkg.size_to_str(10000) # => '10.0 k'
 *
 **/
static
VALUE size_to_str(VALUE self, VALUE size) {
	return rb_str_new2(SizeToStr(NUM2INT(size)).c_str());
}

/*
 * call-seq: string_to_bool(text) -> int
 *
 * Parse the string input and return a boolean.
 *
 *   Debian::AptPkg.string_to_bool('yes') # => 1
 *   Debian::AptPkg.string_to_bool('no') # => 0
 *   Debian::AptPkg.string_to_bool('no-recognized') # => -1
 *
 **/
static
VALUE string_to_bool(VALUE self, VALUE text) {
	return INT2FIX(StringToBool(StringValuePtr(text)));
}

/*
 * call-seq: check_domain_list(host, list) -> bool
 *
 * See if the host name given by host is one of the domains given.
 *
 *   Debian::AptPkg.check_domain_list("alioth.debian.org", "debian.net,debian.org") # => true
 *
 **/
static
VALUE check_domain_list(VALUE self, VALUE host, VALUE list) {
	int res = CheckDomainList(StringValuePtr(host), StringValuePtr(list));
	return INT2BOOL(res);
}

void
Init_apt_pkg() {
	/* Base module */
	VALUE rb_mDebian = rb_define_module("Debian");
	VALUE rb_mDebianAptPkg = rb_define_module_under(rb_mDebian, "AptPkg");

	rb_define_singleton_method(rb_mDebianAptPkg, "cmp_version", RUBY_METHOD_FUNC(cmp_version), 2);
	rb_define_singleton_method(rb_mDebianAptPkg, "check_dep", RUBY_METHOD_FUNC(check_dep), 3);
	rb_define_singleton_method(rb_mDebianAptPkg, "uri_to_filename", RUBY_METHOD_FUNC(uri_to_filename), 1);
	rb_define_singleton_method(rb_mDebianAptPkg, "upstream_version", RUBY_METHOD_FUNC(upstream_version), 1);
	rb_define_singleton_method(rb_mDebianAptPkg, "time_to_str", RUBY_METHOD_FUNC(time_to_str), 1);
	rb_define_singleton_method(rb_mDebianAptPkg, "size_to_str", RUBY_METHOD_FUNC(size_to_str), 1);
	rb_define_singleton_method(rb_mDebianAptPkg, "string_to_bool", RUBY_METHOD_FUNC(string_to_bool), 1);
	rb_define_singleton_method(rb_mDebianAptPkg, "check_domain_list", RUBY_METHOD_FUNC(check_domain_list), 2);

	/* Represents less equal operator. */
	rb_define_const(rb_mDebianAptPkg, "LESS_EQ", INT2FIX(pkgCache::Dep::LessEq));
	/* Represents greater equal operator. */
	rb_define_const(rb_mDebianAptPkg, "GREATER_EQ", INT2FIX(pkgCache::Dep::GreaterEq));
	/* Represents less operator. */
	rb_define_const(rb_mDebianAptPkg, "LESS", INT2FIX(pkgCache::Dep::Less));
	/* Represents greater operator. */
	rb_define_const(rb_mDebianAptPkg, "GREATER", INT2FIX(pkgCache::Dep::Greater));
	/* Represents equals operator. */
	rb_define_const(rb_mDebianAptPkg, "EQUALS", INT2FIX(pkgCache::Dep::Equals));
	/* Represents not equals operator. */
	rb_define_const(rb_mDebianAptPkg, "NOT_EQUALS", INT2FIX(pkgCache::Dep::NotEquals));

	/* Represents the version of apt. */
	rb_define_const(rb_mDebianAptPkg, "VERSION", rb_str_new2(pkgVersion));
	/* Represents the version of apt_pkg library. */
	rb_define_const(rb_mDebianAptPkg, "LIB_VERSION", rb_str_new2(pkgLibVersion));
}