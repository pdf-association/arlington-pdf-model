/*
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
 * Contributors: Roman Toda, Frantisek Forgac, Normex
 */
package gcxml;

import java.io.File;

/**
 *
 * @author fero
 */
public class Gcxml {

	/**
	 * gcxml version string
	 */
    public static final String grammar_version = "0.2.7";

    public static void main(String[] args) {
        final String delimiter = "\t";
        String inputFolder = inputFolder = System.getProperty("user.dir") + "/tsv/latest/";
        File folder = new File(inputFolder);
        File[] listOfFiles = folder.listFiles();

        if(args.length > 0){
            String argument = args[0];

            switch (argument){
                case "--all":
                    System.out.println("gcxml " + grammar_version);
                    double[] pdf_versions = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0};
                    for(int i = 0; i < pdf_versions.length; i++ ){
                        XMLCreator xmlcreator = new XMLCreator(listOfFiles, delimiter, String.valueOf(pdf_versions[i]));
                        xmlcreator.convertFile();
                    }
                    TSVUpdater tsv = new TSVUpdater();
                    break;
                case "--conv":
                    System.out.println("gcxml " + grammar_version);
                    if(args.length>1 && (!args[1].isEmpty())){
                        String pdf_version = args[1];
                        XMLCreator xmlcreator = new XMLCreator(listOfFiles, delimiter, pdf_version);
                        xmlcreator.convertFile();
                    }else{
                        System.out.println("No version specified. Valid options for pdf versions are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0 .");
                    }
                    break;
                case "--sin":
                    System.out.println("gcxml " + grammar_version);
                    if(args.length>1 && !args[1].isEmpty()){
                        String version = args[1];
                        if(version.equals("1.0") || version.equals("1.1") || version.equals("1.2")
                            || version.equals("1.3") || version.equals("1.4") || version.equals("1.5")
                            || version.equals("1.6") || version.equals("1.7") || version.equals("2.0")){
                            XMLQuery query = new XMLQuery();
                            query.SinceVersion(version);
                        }else if(version.equals("-all")){
                            XMLQuery query = new XMLQuery();
                            query.SinceVersion();
                        }else{
                            System.out.println("There is no such PDF version. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0");
                        }
                    }else{
                        System.out.println("PDF version was not specified. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0");
                        System.out.println("If you want to display all keys and their version use '-all' as parameter.");
                    }
                    break;
                case "--dep":
                    System.out.println("gcxml " + grammar_version);
                    if(args.length>1 && !args[1].isEmpty()){
                        String version = args[1];
                        if(version.equals("1.0") || version.equals("1.1") || version.equals("1.2")
                            || version.equals("1.3") || version.equals("1.4") || version.equals("1.5")
                            || version.equals("1.6") || version.equals("1.7") || version.equals("2.0")){
                            XMLQuery query = new XMLQuery();
                            query.DeprecatedIn(version);
                        }else if(version.equals("-all")){
                            XMLQuery query = new XMLQuery();
                            query.DeprecatedIn();
                        }else{
                            System.out.println("There is no such PDF version. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0");
                        }
                    }else{
                        System.out.println("PDF version was not specified. Correct values are: 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0");
                        System.out.println("If you want to display all keys and their version use '-all' as parameter.");
                    }
                    break;
                case "--so":
                    System.out.println("gcxml " + grammar_version);
                    XMLQuery query = new XMLQuery();
                    if(query != null){
                        query.SchizophrenicObjects();
                    }
                    break;
                case "--keys":
                    System.out.println("gcxml " + grammar_version);
                    query = new XMLQuery();
                    if(query != null){
                        query.KeyOccurrenceCount();
                    }
                    break;
                 case "--po":
                    System.out.println("gcxml " + grammar_version);
                    if(args.length > 1){
                        query = new XMLQuery();
                        if(query != null){
                            query.PotentialDicts(args[1]);
                        }
                    }else{
                        System.out.println("No keys specified. Expected list of keys, eg.: Key1,Key2,Key3");
                    }
                    break;
                 case "--version":
                     System.out.println("gcxml " + grammar_version);
                     break;
                 case "--tsv":
                     System.out.println("gcxml " + grammar_version);
                     TSVUpdater tsv2 = new TSVUpdater();
                 case "--sc":
                     System.out.println("gcxml " + grammar_version);
                     query = new XMLQuery();
                     query.getSpecialCases();
                     break;
                case "--help":
                    System.out.println("List of available commands:");
                    System.out.println("\t--version : print version information");
                    System.out.println("\t--help : show list of available commands");
                    System.out.println("\t--conv : convert tsv to xml");
                    System.out.println("\t--tsv : create tsv files for each pdf version");
                    System.out.println("\t--sin : return all keys introduced in specified PDF version");
                    System.out.println("\t--dep : return all keys deprecated in specified PDF version");
                    System.out.println("\t--so : return objects that do not have key Type or where it is not required.");
                    System.out.println("\t--keys : return keys and their occurrence count.");
                    System.out.println("\t--po : return list of potential objects based on given keys.");
                    break;
            }
        }else{
            System.out.println("No arguments specified.\n To see all available commands use --help:");
        }
    }
}