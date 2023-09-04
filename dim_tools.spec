%define debug_package %{nil}

Name:           dim_tools
Version:        1.0.1
Release:        1
Summary:        DIM userspace tools
License:        MulanPSL-2.0
URL:            dim_tools
Source0:        %{name}-v%{version}.tar.gz

BuildRequires:  gcc make elfutils-devel openssl-devel kmod-devel kmod-libs
Requires:       elfutils openssl kmod-libs

%description
dim_tools is DIM userspace tools.

%prep
%autosetup -n %{name}-v%{version} -p1

%build
cd ./src && make

%install
cd ./src && make install DESTDIR=$RPM_BUILD_ROOT

%check
cd ./test/test-function && sh test.sh

%files
%attr(555,root,root) %{_bindir}/dim_gen_baseline

%changelog
