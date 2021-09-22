/**
* Gcxml.java
* Copyright 2020 PDF Association, Inc. https://www.pdfa.org
*
* This material is based upon work supported by the Defense Advanced
* Research Projects Agency (DARPA) under Contract No. HR001119C0079.
* Any opinions, findings and conclusions or recommendations expressed
* in this material are those of the author(s) and do not necessarily
* reflect the views of the Defense Advanced Research Projects Agency
* (DARPA). Approved for public release.
*
* SPDX-License-Identifier: Apache-2.0
* Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
*/
package gcxml;

import java.io.File;

/**
 * @author fero
 */
public class Gcxml {
    /**
     * GCXML version string
     */ 
    public static final String Gcxml_version = "0.5.0";

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        final char delimiter = '\t';
        
        String inputFolder = System.getProperty("user.dir") + "/tsv/latest/";
        File folder = new File(inputFolder);
        File[] listOfFiles = folder.listFiles();

        if (args.length > 0) {
            String argument = args[0];
            System.out.println("gcxml " + Gcxml_version);
            switch (argument) {
                // run -xml and -tsv at once for all pdf versions
                case "-all":
                    for (int i = 0; i < TSVHandler.pdf_version.length; i++ ) {
                        XMLCreator xmlcreator = new XMLCreator(listOfFiles, delimiter);
                        xmlcreator.createXML(String.valueOf(TSVHandler.pdf_version[i]));
                    }
                    TSVHandler tsv = new TSVHandler();
                    tsv.createAllVersionsTSV();
                    break;
                    
                // create grammar in XML format for a specific PDF version from the latest TSV files
                case "-xml":
                    if ((args.length > 1) && (!args[1].isEmpty())) {
                        String pdf_version = args[1];
                        XMLCreator xmlcreator = new XMLCreator(listOfFiles, delimiter);
                        xmlcreator.createXML(pdf_version);
                    }
                    else {
                        for (int i = 0; i < TSVHandler.pdf_version.length; i++ ) {
                            XMLCreator xmlcreator = new XMLCreator(listOfFiles, delimiter);
                            xmlcreator.createXML(String.valueOf(TSVHandler.pdf_version[i]));
                        }
                    }
                    break;
                    
                // display keys introduced in pdf version x.x or "-all"
                case "-sin":
                    if ((args.length > 1) && !args[1].isEmpty()) {
                        String version = args[1];
                        if (version.equals("1.0") || version.equals("1.1") || version.equals("1.2")
                                || version.equals("1.3") || version.equals("1.4") || version.equals("1.5")
                                || version.equals("1.6") || version.equals("1.7") || version.equals("2.0")) {
                            XMLQuery query = new XMLQuery();
                            query.SinceVersion(version);
                        }
                        else if (version.equals("-all")) {
                            XMLQuery query = new XMLQuery();
                            query.SinceVersion();
                        }
                        else {
                            System.out.println("There is no such PDF version. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0 or -all");
                        }
                    }
                    else {
                        System.out.println("PDF version was not specified. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0 or -all.");
                        System.out.println("If you want to display all keys and their version use '-all' as parameter.");
                    }
                    break;
                    
                // display keys deprecated in pdf version x.x or all
                case "-dep":
                    if ((args.length > 1) && !args[1].isEmpty()) {
                        String version = args[1];
                        if (version.equals("1.0") || version.equals("1.1") || version.equals("1.2")
                                || version.equals("1.3") || version.equals("1.4") || version.equals("1.5")
                                || version.equals("1.6") || version.equals("1.7") || version.equals("2.0")) {
                            XMLQuery query = new XMLQuery();
                            query.DeprecatedIn(version);
                        }
                        else if (version.equals("-all")) {
                            XMLQuery query = new XMLQuery();
                            query.DeprecatedIn();
                        }
                        else {
                            System.out.println("There is no such PDF version. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0 or -all.");
                        }
                    }
                    else {
                        System.out.println("PDF version was not specified. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0 or -all.");
                        System.out.println("If you want to display all keys and their version use '-all' as parameter.");
                    }
                    break;
                    
                case "-so":
                    XMLQuery query = new XMLQuery();
                    query.SchizophrenicObjects();
                    break;

                case "-kc":
                    query = new XMLQuery();
                    query.KeyOccurrenceCount();
                    break;

                case "-po":
                    if (args.length > 1) {
                        query = new XMLQuery();
                        query.PotentialDicts(args[1]);
                    }
                    else {
                        System.out.println("No keys specified. Expected list of keys, eg.: Key1,Key2,Key3");
                    }
                    break;
                    
                case "-version":
                    break;

                case "-tsv":
                    TSVHandler tsv2 = new TSVHandler();
                    tsv2.createAllVersionsTSV();
                    break;

                case "-sc":
                    query = new XMLQuery();
                    query.getSpecialCases();
                    break;

                case "-help":
                default:
                    showHelp();
                    break;
            }
        }
        else {
            showHelp();
        }
    }

    /**
     * Command line help
     */
    private static void showHelp() {
        // general info about the gxcml
        System.out.println("GENERAL:");
        System.out.println("\t-version\t\tprint version information (current: " + Gcxml_version + ")");
        System.out.println("\t-help\t\t\tshow list of available commands");
        // converting to other formats
        System.out.println("CONVERSIONS:");
        System.out.println("\t-all\t\t\tconvert latest TSV to XML and TSV sub-versions for each specific PDF version");
        System.out.println("\t-xml <version>\t\tconvert TSV to XML for specified PDF version (or all if no version is specified)");
        System.out.println("\t-tsv\t\t\tcreate TSV files for each PDF version");
        // grammar queries using the xml files
        System.out.println("QUERIES:");
        System.out.println("\t-sin <version | -all>\treturn all keys introduced in (\"since\") a specified PDF version (or all)");
        System.out.println("\t-dep <version | -all>\treturn all keys deprecated in a specified PDF version (or all)");
        System.out.println("\t-kc\t\t\treturn every key name and their occurrence counts for each version of PDF");
        System.out.println("\t-po key<,key1,...>\treturn list of potential objects based on a set of given keys for each version of PDF");
        System.out.println("\t-sc\t\t\tlist special cases for every PDF version");
        System.out.println("\t-so\t\t\treturn objects that are not defined to have key Type, or where the Type key is specified as optional");
        System.out.println("Note: output might be too long to display in terminal, so it is recommended to redirect the output to file (eg <command> > report.txt)");
    }
}
