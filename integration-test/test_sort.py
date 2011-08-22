#!/usr/bin/env python

from joinxtest import JoinxTest, main
import unittest

class TestSort(JoinxTest, unittest.TestCase):

    def test_sort(self):
        input_files = self.inputFiles("unsorted*.bed")
        expected_file = self.inputFiles("expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        # test normal and stable sort
        for arg in ["", "-s"]:
            params = [ "sort", "-o", output_file, arg ]
            params.extend(input_files)
            rv, err = self.joinx(params)
            self.assertEqual(0, rv)
            self.assertEqual('', err)
            self.assertFilesEqual(expected_file, output_file)

    def test_sort_tmp(self):
        input_files = self.inputFiles("unsorted*.bed")
        expected_file = self.inputFiles("expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        lines = sum([len(open(x).readlines()) for x in input_files])
        max_lines = lines / 10

        # test normal and stable sort
        params = [ "sort", "-M", str(max_lines), "-o", output_file ]
        params.extend(input_files)
        rv, err = self.joinx(params)
        self.assertEqual(0, rv)
        self.assertEqual('', err)
        self.assertFilesEqual(expected_file, output_file)

    def test_compression(self):
        input_files = self.inputFiles("unsorted*.bed")
        expected_file = self.inputFiles("expected-sort.bed")[0]
        output_file = self.tempFile("output.bed")

        # test none, gzip, bzip2, and zlib compression
        for arg in ["", "-C g", "-C b", "-C z"]:
            params = [ "sort", "-o", output_file, arg ]
            params.extend(input_files)
            rv, err = self.joinx(params)
            self.assertEqual(0, rv)
            self.assertEqual('', err)
            self.assertFilesEqual(expected_file, output_file)

    def test_sort_vcf(self):
        # currently only 1 vcf file at a time can be sorted, as it is trickier
        # to merge vcf than bed.
        input_file = self.inputFiles("unsorted0.vcf")[0]
        expected_file = self.inputFiles("expected-sort.vcf")[0]
        output_file = self.tempFile("output.bed")
        params = [ "sort", "-o", output_file, input_file ]
        rv, err = self.joinx(params)
        self.assertEqual(0, rv)
        self.assertEqual('', err)
        self.assertFilesEqual(expected_file, output_file)

        



if __name__ == "__main__":
    main()

