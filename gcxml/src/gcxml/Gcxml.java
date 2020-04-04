/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package gcxml;

import java.io.File;

/**
 *
 * @author fero
 */
public class Gcxml {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        final String delimiter = "\t";
        
        if(args.length > 0){
            String argument = args[0];

            switch (argument){
                case "--conv":   
                    if(args.length>1 && (!args[1].isEmpty() && !("-all".equals(args[1])))){
                        String fileName = args[1];

                        XMLCreator xmlcreator = new XMLCreator();
                        xmlcreator.convertFile(fileName, delimiter);
                    }else if(args.length>1 && "-all".equals(args[1])){
                        String inputFolder = inputFolder = System.getProperty("user.dir") + "/csv/";
                        File folder = new File(inputFolder);
                        File[] listOfFiles = folder.listFiles();
                        for (File file : listOfFiles) {
                            if (file.isFile() && file.canRead() && file.exists()) {
                                XMLCreator xmlcreator = new XMLCreator();
                                xmlcreator.convertFile(file.getName().substring(0, file.getName().length()-4), delimiter);
                            }
                        }
                    }else{
                        System.out.println("No file name specified. To convert all files in directory, use -all");
                    }
                    break;
                case "--sin":
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
                case "--help":
                    System.out.println("List of available commands");
                    System.out.println("\t--help : shows list of available commands");
                    System.out.println("\t--conv : converts csv to xml");
                    System.out.println("\t--sin : returns all keys introduced in specified PDF version");
                    break;
            }
        }else{
            System.out.println("No arguments specified.\n To see all available commands use --help:");
        }
    }
}