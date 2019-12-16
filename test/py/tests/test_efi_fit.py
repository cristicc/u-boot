# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2019, Cristian Ciocaltea <cristian.ciocaltea@gmail.com>

# Test launching UEFI binaries from FIT images.

import os
import pytest
import u_boot_utils as util

# Define the parametrized ITS data to be used for FIT image generation.
its_data = '''
/dts-v1/;

/ {
    description = "EFI image with FDT blob";
    #address-cells = <1>;

    images {
        efi {
            description = "Sandbox EFI";
            data = /incbin/("%(efi-bin)s");
            type = "%(kernel-type)s";
            arch = "sandbox";
            os = "efi";
            compression = "%(efi-comp)s";
            load = <0x0>;
            entry = <0x0>;
        };
        fdt {
            description = "Sandbox FDT";
            data = /incbin/("%(fdt-bin)s");
            type = "flat_dt";
            arch = "sandbox";
            compression = "%(fdt-comp)s";
        };
    };

    configurations {
        default = "config-efi-fdt";
        config-efi-fdt {
            description = "EFI FIT w/ FDT";
            kernel = "efi";
            fdt = "fdt";
        };
        config-efi-nofdt {
            description = "EFI FIT w/o FDT";
            kernel = "efi";
        };
    };
};
'''

# Define the parametrized FDT data.
fdt_data = '''
/dts-v1/;

/ {
    model = "Sandbox %(fdt_type) EFI FIT Boot Test ";
    compatible = "sandbox";

    reset@0 {
        compatible = "sandbox,reset";
    };
};
'''

@pytest.mark.boardspec('sandbox')
@pytest.mark.buildconfigspec('bootm_efi')
@pytest.mark.buildconfigspec('cmd_bootefi_hello_compile')
@pytest.mark.requiredtool('dtc')
def test_efi_fit(u_boot_console):
    """Test handling of UEFI binaries inside FIT images.

    The tests are trying to launch U-Boot's helloworld.efi embedded into
    FIT images, in uncompressed or gzip compressed format.

    Additionally, a sample FDT blob is created and embedded into the above
    mentioned FIT images, in uncompressed or gzip compressed format.

    The following test cases are currently defined and enabled:
    - Launch uncompressed FIT EFI & FIT FDT
    - Launch compressed FIT EFI & FIT FDT
    - Launch uncompressed FIT EFI & internal FDT
    - Launch compressed FIT EFI & internal FDT
    """

    def make_fpath(fname):
        """Compute the path of a given (temporary) file.

        Args:
            fname: The name of a file within U-Boot build dir.
        Return:
            The computed file path.
        """
        return os.path.join(u_boot_console.config.build_dir, fname)

    def make_efi(fname, comp):
        """Create an UEFI binary.

        This simply copies lib/efi_loader/helloworld.efi into U-Boot
        build dir and, optionally, compresses the file using gzip.

        Args:
            fname: The target file name within U-Boot build dir.
            comp: Flag to enable gzip compression.
        Return:
            The path of the created file.
        """
        bin_path = make_fpath(fname)
        os.system('cp %s %s' % (make_fpath('lib/efi_loader/helloworld.efi'), bin_path))
        if comp:
            util.run_and_log(u_boot_console, ['gzip', '-f', bin_path])
            bin_path += '.gz'
        return bin_path

    def make_dtb(fdt_type, comp):
        """Create a sample DTB file.

        Creates a DTS file and compiles it to a DTB.

        Args:
            fdt_type: The type of the FDT, i.e. internal, user.
            comp: Flag to enable gzip compression.
        Returns:
            The path of the created file.
        """
        dts = make_fpath('test-efi-fit-sandbox-%s.dts' % fdt_type)
        dtb = make_fpath('test-efi-fit-sandbox-%s.dtb' % fdt_type)
        with open(dts, 'w') as fd:
            fd.write(fdt_data)
        util.run_and_log(u_boot_console, ['dtc', dts, '-O', 'dtb', '-o', dtb])
        if comp:
            util.run_and_log(u_boot_console, ['gzip', '-f', dtb])
            dtb += '.gz'
        return dtb

    def make_fit(efi_comp, fdt_comp):
        """Create a sample FIT image.

        Runs 'mkimage' to create a FIT image within U-Boot build dir.
        Args:
            efi_comp: Enable gzip compression for the EFI binary.
            fdt_comp: Enable gzip compression for the FDT blob.
        Return:
            The path of the created file.
        """
        # Generate resources referenced by ITS.
        its_params = {
            'efi-bin' : os.path.basename(
                            make_efi('test-efi-fit-sandbox.efi', efi_comp)),
            'kernel-type' : 'kernel' if efi_comp else 'kernel_noload',
            'efi-comp' : 'gzip' if efi_comp else 'none',
            'fdt-bin' : os.path.basename(make_dtb('user', fdt_comp)),
            'fdt-comp' : 'gzip' if fdt_comp else 'none',
        }

        # Generate a test ITS file.
        its_path = make_fpath('test-efi-fit.its')
        with open(its_path, 'w') as fd:
            fd.write(its_data % its_params)

        # Build the test ITS.
        fit_path = make_fpath('test-efi-fit.fit')
        util.run_and_log(u_boot_console,
            [make_fpath('tools/mkimage'), '-f', its_path, fit_path])
        return fit_path

    def file_size(fpath):
        """Get the size of a file.

        Args:
            fname: Path of the file to check.
        Return:
            The size of file in bytes.
        """
        return os.stat(fpath).st_size

    def launch_efi(enable_fdt, enable_comp):
        """Launch a UEFI binary from a FIT image.

        Creates a test FIT image containing a fake UEFI binary and tries
        to run it via the 'bootm' command in the U-Boot console.

        Args:
            enable_fdt: Flag to enable using the FDT blob inside FIT image.
            enable_comp: Flag to enable gzip compression on EFI and FDT content.
        """
        # Create test FIT image.
        fit_img_path = make_fit(enable_comp, enable_comp)

        # Select boot configuration.
        fit_config = 'config-efi-fdt' if enable_fdt else 'config-efi-nofdt'

        # Run U-Boot commands.
        with u_boot_console.log.section('Boot EFI FIT %s' % fit_config):
            output = u_boot_console.run_command(
                'host load hostfs - $kernel_addr_r %s' % fit_img_path)

            expected_text = '%d bytes read' % file_size(fit_img_path)
            assert(expected_text in output)

            output = u_boot_console.run_command(
                'bootm ${kernel_addr_r}#%s' % fit_config)

            if enable_fdt:
                expected_text = 'Booting using the fdt blob'
                assert(expected_text in output)

            expected_text = 'Hello, world'
            assert expected_text in output
            expected_text = '## Application terminated, r = 0'
            assert expected_text in output

    try:
        # Use our own device tree file, will be restored afterwards.
        control_dtb = make_dtb('internal', False)
        old_dtb = u_boot_console.config.dtb
        u_boot_console.config.dtb = control_dtb

        # Run tests
        # - fdt ON, gzip OFF
        launch_efi(True, False)
        # - fdt ON, gzip ON
        launch_efi(True, True)
        # - fdt OFF, gzip OFF
        launch_efi(False, False)
        # - fdt OFF, gzip ON
        launch_efi(False, True)

    finally:
        # Go back to the original U-Boot with the correct dtb.
        u_boot_console.config.dtb = old_dtb
        u_boot_console.restart_uboot()
